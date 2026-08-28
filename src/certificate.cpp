#include "certificate.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include "crypto.h"
#include "securestore.h"

namespace {

const char *CertFileName = "identity-cert.pem";
const char *KeyFileName = "identity-key.pem";

// Ten years. The certificate is an identity, not a credential that anybody
// rotates, and a device whose transfers stop working one morning because a
// self-signed certificate aged out would be a bug with no upside.
const long ValiditySeconds = 60L * 60L * 24L * 365L * 10L;

// The subject is deliberately generic. It is broadcast to every device on the
// network in the TLS handshake, and the alias - which the user chooses, and
// may well be their name - has no business being in there. It also cannot
// change, while the alias can.
const char *SubjectCommonName = "LocalSend";

// Writes `data` to `path` with owner-only permissions from the moment the
// file exists. The file is created empty and chmodded before a single byte of
// key material goes into it, so there is no window in which the key is
// readable by anyone else.
bool writePrivate(const QString &path, const QByteArray &data)
{
    // Through the store, so the key is encrypted whenever the platform will
    // hold a key for us. It sets owner-only permissions either way.
    return SecureStore::instance().write(path, data);
}

QByteArray readAll(const QString &path)
{
    return SecureStore::instance().read(path);
}

} // namespace

Certificate::Certificate()
{
}

bool Certificate::isValid() const
{
    return !m_certificate.isNull() && !m_key.isNull() && !m_fingerprint.isEmpty();
}

QSslCertificate Certificate::certificate() const { return m_certificate; }
QSslKey Certificate::privateKey() const { return m_key; }
QString Certificate::fingerprint() const { return m_fingerprint; }
QString Certificate::lastError() const { return m_lastError; }

QString Certificate::fingerprintOf(const QSslCertificate &certificate)
{
    if (certificate.isNull())
        return QString();
    // DER, not PEM: PEM is base64 with line breaks, and two encoders can
    // produce different bytes for the same certificate.
    return Crypto::sha256Hex(certificate.toDer());
}

bool Certificate::ensure(const QString &directory)
{
    if (isValid())
        return true;

    if (!QDir().mkpath(directory)) {
        m_lastError = QStringLiteral("cannot create %1").arg(directory);
        return false;
    }

    const QString certPath = directory + QLatin1Char('/') + QLatin1String(CertFileName);
    const QString keyPath = directory + QLatin1Char('/') + QLatin1String(KeyFileName);

    if (QFile::exists(certPath) && QFile::exists(keyPath)) {
        if (load(certPath, keyPath))
            return true;
        // A pair we cannot read is worse than no pair: it would leave us
        // announcing a fingerprint we cannot prove. Start again.
        QFile::remove(certPath);
        QFile::remove(keyPath);
    }

    return generate(certPath, keyPath);
}

bool Certificate::load(const QString &certPath, const QString &keyPath)
{
    const QByteArray certPem = readAll(certPath);
    const QByteArray keyPem = readAll(keyPath);
    if (certPem.isEmpty() || keyPem.isEmpty()) {
        m_lastError = QStringLiteral("stored identity is unreadable");
        return false;
    }

    const QSslCertificate certificate(certPem, QSsl::Pem);
    const QSslKey key(keyPem, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey);

    if (certificate.isNull() || key.isNull()) {
        m_lastError = QStringLiteral("stored identity is not a usable key pair");
        return false;
    }

    // A key file that became group- or world-readable at some point is not
    // something to carry on using quietly.
    QFile::setPermissions(keyPath, QFile::ReadOwner | QFile::WriteOwner);

    m_certificate = certificate;
    m_key = key;
    m_fingerprint = fingerprintOf(certificate);
    m_lastError.clear();
    return true;
}

bool Certificate::generate(const QString &certPath, const QString &keyPath)
{
    m_lastError = QStringLiteral("unknown failure");

    EVP_PKEY *pkey = 0;
    EVP_PKEY_CTX *keyContext = 0;
    X509 *x509 = 0;
    BIGNUM *serialNumber = 0;
    ASN1_INTEGER *serial = 0;
    BIO *bio = 0;
    bool success = false;

    // P-256 rather than RSA. Key generation is milliseconds instead of
    // seconds, which matters when it happens during the very first launch,
    // and every TLS 1.2 stack on the other side of a LocalSend transfer
    // handles ECDSA.
    keyContext = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, 0);
    if (!keyContext)
        goto done;
    if (EVP_PKEY_keygen_init(keyContext) <= 0)
        goto done;
    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(keyContext,
                                               NID_X9_62_prime256v1) <= 0) {
        goto done;
    }
    if (EVP_PKEY_keygen(keyContext, &pkey) <= 0)
        goto done;

    x509 = X509_new();
    if (!x509)
        goto done;

    // Version 3, which is what the extensions below require.
    if (X509_set_version(x509, 2) != 1)
        goto done;

    {
        // A random serial rather than 1. Serials are meant to be unique per
        // issuer, and every install here is its own issuer.
        const QByteArray serialBytes = Crypto::randomBytes(16);
        if (serialBytes.isEmpty()) {
            m_lastError = QStringLiteral("no secure random source");
            goto done;
        }
        serialNumber = BN_bin2bn(
            reinterpret_cast<const unsigned char *>(serialBytes.constData()),
            serialBytes.size(), 0);
        if (!serialNumber)
            goto done;
        // Clear the top bit so the DER integer is unambiguously positive.
        BN_clear_bit(serialNumber, BN_num_bits(serialNumber) - 1);
        serial = BN_to_ASN1_INTEGER(serialNumber, 0);
        if (!serial || X509_set_serialNumber(x509, serial) != 1)
            goto done;
    }

    if (!X509_gmtime_adj(X509_getm_notBefore(x509), 0))
        goto done;
    if (!X509_gmtime_adj(X509_getm_notAfter(x509), ValiditySeconds))
        goto done;

    if (X509_set_pubkey(x509, pkey) != 1)
        goto done;

    {
        X509_NAME *name = X509_get_subject_name(x509);
        if (!name)
            goto done;
        X509_NAME_add_entry_by_txt(
            name, "CN", MBSTRING_ASC,
            reinterpret_cast<const unsigned char *>(SubjectCommonName), -1, -1, 0);
        X509_NAME_add_entry_by_txt(
            name, "O", MBSTRING_ASC,
            reinterpret_cast<const unsigned char *>(SubjectCommonName), -1, -1, 0);
        // Self-signed: the issuer is the subject.
        if (X509_set_issuer_name(x509, name) != 1)
            goto done;
    }

    {
        // Well-formed enough that a strict TLS stack has nothing to object to
        // beyond the self-signing itself, which is the part we expect the
        // peer to resolve by checking the fingerprint.
        X509V3_CTX context;
        X509V3_set_ctx_nodb(&context);
        X509V3_set_ctx(&context, x509, x509, 0, 0, 0);

        struct { int nid; const char *value; } extensions[] = {
            { NID_basic_constraints, "critical,CA:TRUE" },
            { NID_key_usage, "critical,digitalSignature,keyCertSign" },
            { NID_ext_key_usage, "serverAuth,clientAuth" },
            { NID_subject_key_identifier, "hash" },
        };

        for (unsigned i = 0; i < sizeof(extensions) / sizeof(extensions[0]); ++i) {
            X509_EXTENSION *extension = X509V3_EXT_conf_nid(
                0, &context, extensions[i].nid,
                const_cast<char *>(extensions[i].value));
            if (!extension)
                goto done;
            X509_add_ext(x509, extension, -1);
            X509_EXTENSION_free(extension);
        }
    }

    if (X509_sign(x509, pkey, EVP_sha256()) == 0)
        goto done;

    {
        QByteArray certPem;
        QByteArray keyPem;

        bio = BIO_new(BIO_s_mem());
        if (!bio || PEM_write_bio_X509(bio, x509) != 1)
            goto done;
        {
            char *data = 0;
            const long length = BIO_get_mem_data(bio, &data);
            certPem = QByteArray(data, int(length));
        }
        BIO_free(bio);

        // Unencrypted on purpose. A passphrase would have to be stored beside
        // the key to be usable unattended, which protects nothing; what
        // actually protects it is the sandbox and 0600.
        bio = BIO_new(BIO_s_mem());
        if (!bio || PEM_write_bio_PrivateKey(bio, pkey, 0, 0, 0, 0, 0) != 1)
            goto done;
        {
            char *data = 0;
            const long length = BIO_get_mem_data(bio, &data);
            keyPem = QByteArray(data, int(length));
        }
        BIO_free(bio);
        bio = 0;

        const QSslCertificate certificate(certPem, QSsl::Pem);
        const QSslKey key(keyPem, QSsl::Ec, QSsl::Pem, QSsl::PrivateKey);
        if (certificate.isNull() || key.isNull()) {
            m_lastError = QStringLiteral("Qt rejected the generated key pair");
            goto done;
        }

        if (!writePrivate(keyPath, keyPem) || !writePrivate(certPath, certPem)) {
            m_lastError = QStringLiteral("cannot store the identity");
            goto done;
        }

        m_certificate = certificate;
        m_key = key;
        m_fingerprint = fingerprintOf(certificate);
        m_lastError.clear();
        success = true;
    }

done:
    if (bio)
        BIO_free(bio);
    if (serial)
        ASN1_INTEGER_free(serial);
    if (serialNumber)
        BN_free(serialNumber);
    if (x509)
        X509_free(x509);
    if (pkey)
        EVP_PKEY_free(pkey);
    if (keyContext)
        EVP_PKEY_CTX_free(keyContext);

    return success;
}
