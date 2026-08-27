#include <QSslCertificate>
#include <QTemporaryDir>
#include <QtTest>

#include "certificate.h"
#include "crypto.h"
#include "ratelimiter.h"

// The security primitives, tested for the properties they are relied on for
// rather than for "does it return something".
class TestSecurity : public QObject
{
    Q_OBJECT

private slots:
    void randomIsAvailableAndUnpredictable();
    void comparisonIsExactAndLengthSafe();
    void derivedKeysDependOnBothInputs();

    void generatesAUsableIdentity();
    void keepsTheSameIdentityAcrossRestarts();
    void storesThePrivateKeyOwnerOnly();
    void fingerprintIsTheHashOfTheDerCertificate();
    void replacesAnIdentityItCannotRead();

    void rateLimiterLetsHonestMistakesThrough();
    void rateLimiterBacksOffAndRecovers();
    void rateLimiterForgetsOnSuccess();
};

void TestSecurity::randomIsAvailableAndUnpredictable()
{
    QVERIFY(Crypto::isAvailable());

    const QByteArray first = Crypto::randomBytes(32);
    const QByteArray second = Crypto::randomBytes(32);

    QCOMPARE(first.size(), 32);
    QCOMPARE(second.size(), 32);
    QVERIFY(first != second);

    // A generator that has quietly failed open would return zeroes.
    QVERIFY(first != QByteArray(32, '\0'));

    QCOMPARE(Crypto::randomHex(16).length(), 32);
    QVERIFY(Crypto::randomBytes(0).isEmpty());
    QVERIFY(Crypto::randomBytes(-1).isEmpty());
}

void TestSecurity::comparisonIsExactAndLengthSafe()
{
    QVERIFY(Crypto::equals(QByteArray("token"), QByteArray("token")));
    QVERIFY(!Crypto::equals(QByteArray("token"), QByteArray("tokem")));
    // Differing lengths must not read past the shorter buffer.
    QVERIFY(!Crypto::equals(QByteArray("token"), QByteArray("token-longer")));
    QVERIFY(!Crypto::equals(QByteArray(), QByteArray("x")));
    QVERIFY(Crypto::equals(QByteArray(), QByteArray()));

    QVERIFY(Crypto::equals(QStringLiteral("1234"), QStringLiteral("1234")));
    QVERIFY(!Crypto::equals(QStringLiteral("1234"), QStringLiteral("1235")));
}

void TestSecurity::derivedKeysDependOnBothInputs()
{
    const QByteArray salt = Crypto::randomBytes(16);
    const QByteArray other = Crypto::randomBytes(16);

    const QByteArray key = Crypto::deriveKey(QStringLiteral("4269"), salt, 1000, 32);
    QCOMPARE(key.size(), 32);

    // Deterministic for the same inputs, or verifying a PIN could never work.
    QCOMPARE(Crypto::deriveKey(QStringLiteral("4269"), salt, 1000, 32), key);

    // And sensitive to each input independently, or the salt would be
    // decoration and the same PIN would hash alike on every device.
    QVERIFY(Crypto::deriveKey(QStringLiteral("4270"), salt, 1000, 32) != key);
    QVERIFY(Crypto::deriveKey(QStringLiteral("4269"), other, 1000, 32) != key);
    QVERIFY(Crypto::deriveKey(QStringLiteral("4269"), salt, 2000, 32) != key);
}

void TestSecurity::generatesAUsableIdentity()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    Certificate identity;
    QVERIFY2(identity.ensure(directory.path()), qPrintable(identity.lastError()));
    QVERIFY(identity.isValid());

    QVERIFY(!identity.certificate().isNull());
    QVERIFY(!identity.privateKey().isNull());
    QCOMPARE(identity.fingerprint().length(), 64);

    // Self-signed, which is what makes the fingerprint the only thing worth
    // checking on the other side.
    QCOMPARE(identity.certificate().issuerInfo(QSslCertificate::CommonName),
             identity.certificate().subjectInfo(QSslCertificate::CommonName));

    // Valid now and for a long time: an identity that expires is an app that
    // stops working one morning for no visible reason.
    const QDateTime now = QDateTime::currentDateTimeUtc();
    QVERIFY(identity.certificate().effectiveDate() <= now);
    QVERIFY(identity.certificate().expiryDate() > now.addYears(5));
}

void TestSecurity::keepsTheSameIdentityAcrossRestarts()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    Certificate first;
    QVERIFY(first.ensure(directory.path()));

    // Peers recognise this device by its fingerprint. Regenerating on every
    // launch would make it a stranger every time.
    Certificate second;
    QVERIFY2(second.ensure(directory.path()), qPrintable(second.lastError()));
    QCOMPARE(second.fingerprint(), first.fingerprint());
    QCOMPARE(second.certificate().toDer(), first.certificate().toDer());

    // Two different devices must not collide.
    QTemporaryDir elsewhere;
    Certificate other;
    QVERIFY(other.ensure(elsewhere.path()));
    QVERIFY(other.fingerprint() != first.fingerprint());
}

void TestSecurity::storesThePrivateKeyOwnerOnly()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    Certificate identity;
    QVERIFY(identity.ensure(directory.path()));

    const QFile::Permissions permissions =
        QFile::permissions(directory.path() + QStringLiteral("/identity-key.pem"));

    QVERIFY(permissions & QFile::ReadOwner);
    QVERIFY(!(permissions & QFile::ReadGroup));
    QVERIFY(!(permissions & QFile::ReadOther));
    QVERIFY(!(permissions & QFile::WriteGroup));
    QVERIFY(!(permissions & QFile::WriteOther));
}

void TestSecurity::fingerprintIsTheHashOfTheDerCertificate()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    Certificate identity;
    QVERIFY(identity.ensure(directory.path()));

    // The protocol says SHA-256 of the certificate, and a peer computes it
    // from what it received on the wire. If our idea of it were computed any
    // other way, every HTTPS transfer would be refused.
    QCOMPARE(identity.fingerprint(),
             Crypto::sha256Hex(identity.certificate().toDer()));
    QCOMPARE(Certificate::fingerprintOf(identity.certificate()),
             identity.fingerprint());

    QVERIFY(Certificate::fingerprintOf(QSslCertificate()).isEmpty());
}

void TestSecurity::replacesAnIdentityItCannotRead()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString keyPath = directory.path() + QStringLiteral("/identity-key.pem");
    const QString certPath = directory.path() + QStringLiteral("/identity-cert.pem");

    Certificate first;
    QVERIFY(first.ensure(directory.path()));

    // A half-written or corrupted pair must not leave us announcing a
    // fingerprint we cannot prove ownership of; the only safe move is to
    // start over.
    QFile key(keyPath);
    QVERIFY(key.open(QIODevice::WriteOnly | QIODevice::Truncate));
    key.write("not a key");
    key.close();

    Certificate second;
    QVERIFY2(second.ensure(directory.path()), qPrintable(second.lastError()));
    QVERIFY(second.isValid());
    QVERIFY(second.fingerprint() != first.fingerprint());
    QVERIFY(QFile::exists(certPath));
}

void TestSecurity::rateLimiterLetsHonestMistakesThrough()
{
    RateLimiter limiter;
    const QString peer = QStringLiteral("192.168.1.5");

    QVERIFY(limiter.allow(peer));

    // Somebody mistyping their own PIN a couple of times must not be made to
    // wait; the limiter is aimed at a machine, not at a person.
    limiter.recordFailure(peer);
    QVERIFY(limiter.allow(peer));
    limiter.recordFailure(peer);
    QVERIFY(limiter.allow(peer));
    limiter.recordFailure(peer);
    QVERIFY(limiter.allow(peer));
    QCOMPARE(limiter.retryAfter(peer), 0);
}

void TestSecurity::rateLimiterBacksOffAndRecovers()
{
    RateLimiter limiter;
    const QString peer = QStringLiteral("192.168.1.6");
    const QString innocent = QStringLiteral("192.168.1.7");

    for (int i = 0; i < 6; ++i)
        limiter.recordFailure(peer);

    QVERIFY(!limiter.allow(peer));
    QVERIFY(limiter.retryAfter(peer) > 0);

    // One address being throttled must not affect any other.
    QVERIFY(limiter.allow(innocent));

    // Enough guesses and the wait becomes long enough that an exhaustive
    // search is not a plan any more.
    for (int i = 0; i < 10; ++i)
        limiter.recordFailure(peer);
    QVERIFY(limiter.retryAfter(peer) > 60);

    // And it does expire: a device is never locked out permanently.
    limiter.setClockOffset(20 * 60 * 1000);
    QVERIFY(limiter.allow(peer));
    QCOMPARE(limiter.retryAfter(peer), 0);
}

void TestSecurity::rateLimiterForgetsOnSuccess()
{
    RateLimiter limiter;
    const QString peer = QStringLiteral("192.168.1.8");

    for (int i = 0; i < 6; ++i)
        limiter.recordFailure(peer);
    QVERIFY(!limiter.allow(peer));

    limiter.recordSuccess(peer);
    QVERIFY(limiter.allow(peer));
    QCOMPARE(limiter.retryAfter(peer), 0);
}

QTEST_MAIN(TestSecurity)
#include "tst_security.moc"
