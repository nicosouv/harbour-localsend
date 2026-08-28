#include "crypto.h"

#include <QCryptographicHash>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace Crypto {

namespace {

// Latches once the CSPRNG has answered. It never becomes unavailable again on
// a running system, so this only records that the pool was reachable at all.
bool s_seen = false;

} // namespace

bool isAvailable()
{
    if (s_seen)
        return true;
    unsigned char probe = 0;
    if (RAND_bytes(&probe, 1) == 1)
        s_seen = true;
    return s_seen;
}

QByteArray randomBytes(int count)
{
    if (count <= 0)
        return QByteArray();

    QByteArray buffer(count, '\0');
    if (RAND_bytes(reinterpret_cast<unsigned char *>(buffer.data()), count) != 1) {
        // Deliberately empty rather than degraded. Everything that asks for
        // random bytes here is producing a secret, and a predictable secret
        // is worse than a failed operation.
        return QByteArray();
    }

    s_seen = true;
    return buffer;
}

QString randomHex(int byteCount)
{
    return QString::fromLatin1(randomBytes(byteCount).toHex());
}

bool equals(const QByteArray &left, const QByteArray &right)
{
    // Lengths are not secret, and CRYPTO_memcmp needs them equal.
    if (left.size() != right.size())
        return false;
    if (left.isEmpty())
        return true;
    return CRYPTO_memcmp(left.constData(), right.constData(),
                         size_t(left.size())) == 0;
}

bool equals(const QString &left, const QString &right)
{
    return equals(left.toUtf8(), right.toUtf8());
}

QByteArray deriveKey(const QString &secret, const QByteArray &salt,
                     int iterations, int length)
{
    if (length <= 0 || iterations <= 0)
        return QByteArray();

    const QByteArray password = secret.toUtf8();
    QByteArray key(length, '\0');

    const int ok = PKCS5_PBKDF2_HMAC(
        password.constData(), password.size(),
        reinterpret_cast<const unsigned char *>(salt.constData()), salt.size(),
        iterations, EVP_sha256(),
        length, reinterpret_cast<unsigned char *>(key.data()));

    return ok == 1 ? key : QByteArray();
}

namespace {

// Standard GCM sizes. The nonce is 96 bits because that is the one length
// GCM uses directly rather than hashing down to, and the tag is the full 128.
const int NonceBytes = 12;
const int TagBytes = 16;

} // namespace

QByteArray encrypt(const QByteArray &plaintext, const QByteArray &key)
{
    if (key.size() != KeyBytes)
        return QByteArray();

    const QByteArray nonce = randomBytes(NonceBytes);
    if (nonce.isEmpty())
        return QByteArray();

    EVP_CIPHER_CTX *context = EVP_CIPHER_CTX_new();
    if (!context)
        return QByteArray();

    QByteArray output;
    QByteArray body(plaintext.size() + TagBytes, '\0');
    int length = 0;
    int total = 0;

    if (EVP_EncryptInit_ex(context, EVP_aes_256_gcm(), 0, 0, 0) != 1)
        goto done;
    if (EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN, NonceBytes, 0) != 1)
        goto done;
    if (EVP_EncryptInit_ex(
            context, 0, 0,
            reinterpret_cast<const unsigned char *>(key.constData()),
            reinterpret_cast<const unsigned char *>(nonce.constData())) != 1) {
        goto done;
    }

    if (!plaintext.isEmpty()) {
        if (EVP_EncryptUpdate(
                context, reinterpret_cast<unsigned char *>(body.data()), &length,
                reinterpret_cast<const unsigned char *>(plaintext.constData()),
                plaintext.size()) != 1) {
            goto done;
        }
        total = length;
    }

    if (EVP_EncryptFinal_ex(
            context, reinterpret_cast<unsigned char *>(body.data()) + total,
            &length) != 1) {
        goto done;
    }
    total += length;

    if (EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_GET_TAG, TagBytes,
                            body.data() + total) != 1) {
        goto done;
    }
    total += TagBytes;

    body.resize(total);
    output = nonce + body;

done:
    EVP_CIPHER_CTX_free(context);
    return output;
}

QByteArray decrypt(const QByteArray &blob, const QByteArray &key)
{
    if (key.size() != KeyBytes || blob.size() < NonceBytes + TagBytes)
        return QByteArray();

    const QByteArray nonce = blob.left(NonceBytes);
    const int bodySize = blob.size() - NonceBytes - TagBytes;
    const char *body = blob.constData() + NonceBytes;
    const char *tag = body + bodySize;

    EVP_CIPHER_CTX *context = EVP_CIPHER_CTX_new();
    if (!context)
        return QByteArray();

    QByteArray plaintext(bodySize, '\0');
    QByteArray output;
    int length = 0;
    int total = 0;

    if (EVP_DecryptInit_ex(context, EVP_aes_256_gcm(), 0, 0, 0) != 1)
        goto done;
    if (EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_IVLEN, NonceBytes, 0) != 1)
        goto done;
    if (EVP_DecryptInit_ex(
            context, 0, 0,
            reinterpret_cast<const unsigned char *>(key.constData()),
            reinterpret_cast<const unsigned char *>(nonce.constData())) != 1) {
        goto done;
    }

    if (bodySize > 0) {
        if (EVP_DecryptUpdate(
                context, reinterpret_cast<unsigned char *>(plaintext.data()),
                &length, reinterpret_cast<const unsigned char *>(body),
                bodySize) != 1) {
            goto done;
        }
        total = length;
    }

    if (EVP_CIPHER_CTX_ctrl(context, EVP_CTRL_GCM_SET_TAG, TagBytes,
                            const_cast<char *>(tag)) != 1) {
        goto done;
    }

    // Returns failure when the tag does not verify, which is the whole point:
    // a blob that has been altered comes back as nothing rather than as
    // plausible-looking rubbish.
    if (EVP_DecryptFinal_ex(
            context, reinterpret_cast<unsigned char *>(plaintext.data()) + total,
            &length) != 1) {
        goto done;
    }
    total += length;

    plaintext.resize(total);
    output = plaintext;

done:
    EVP_CIPHER_CTX_free(context);
    return output;
}

bool equalsFold(const QString &left, const QString &right)
{
    // Case is folded first and the comparison itself stays constant-time.
    // Fingerprints are public values, so this is consistency rather than
    // necessity, but a second comparison routine with different properties is
    // exactly how the wrong one ends up guarding a token one day.
    return equals(left.toUpper(), right.toUpper());
}

QString sha256Hex(const QByteArray &data)
{
    return QString::fromLatin1(
        QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex().toUpper());
}

} // namespace Crypto
