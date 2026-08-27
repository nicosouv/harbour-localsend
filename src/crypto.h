#ifndef CRYPTO_H
#define CRYPTO_H

#include <QByteArray>
#include <QString>

// The cryptographic primitives the app relies on, in one place and backed by
// OpenSSL rather than by Qt.
//
// Qt's own facilities are not enough here for two reasons. QUuid::createUuid()
// reads /dev/urandom when it can and silently falls back to qrand() when it
// cannot, and a session token that came out of qrand() is guessable by anyone
// on the network - a token is a capability, not an identifier. And Qt has no
// password hash and no constant-time comparison at all.
namespace Crypto {

// True when the random source answered at least once. False means every
// token-producing call has failed, and the caller must refuse to proceed
// rather than fall back to something weaker.
bool isAvailable();

// `count` bytes from the CSPRNG. Empty on failure, never a weaker substitute.
QByteArray randomBytes(int count);

// 2 * `byteCount` lowercase hex characters, from randomBytes().
QString randomHex(int byteCount);

// Comparison that does not stop at the first differing byte.
//
// A token check that returns early leaks, in its timing, how much of a guess
// was right, which turns an infeasible search into a per-byte one. The margin
// is small over a network and the fix costs nothing, so there is no argument
// for the naive version.
bool equals(const QByteArray &left, const QByteArray &right);
bool equals(const QString &left, const QString &right);

// The same, ignoring case. For hex fingerprints, whose casing is not fixed by
// the protocol text: the reference implementation emits uppercase, and an
// implementation that emits lowercase is not wrong, only different. Comparing
// them byte-exact makes every peer that chose the other convention
// unreachable, with a network error rather than anything that points at the
// cause.
bool equalsFold(const QString &left, const QString &right);

// PBKDF2-HMAC-SHA256. Used for the PIN, which is short enough that the work
// factor is most of what protects it.
QByteArray deriveKey(const QString &secret, const QByteArray &salt,
                     int iterations, int length);

// SHA-256 of `data` as uppercase hex.
//
// Uppercase because that is what the reference implementation produces and
// therefore what peers announce and compare against:
//
//     sha256(der).iter().map(|byte| format!("{byte:02X}")).collect()
//
// Comparisons still go through equalsFold(), so a peer using the other
// convention works too - but what we *announce* has to match the majority, or
// implementations that pin strictly will refuse us.
QString sha256Hex(const QByteArray &data);

} // namespace Crypto

#endif // CRYPTO_H
