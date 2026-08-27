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
