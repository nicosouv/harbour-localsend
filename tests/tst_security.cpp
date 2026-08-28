#include <QSslCertificate>
#include <QTemporaryDir>
#include <QtTest>

#include "certificate.h"
#include "crypto.h"
#include "knowndevices.h"
#include "ratelimiter.h"
#include "securestore.h"

// The security primitives, tested for the properties they are relied on for
// rather than for "does it return something".
class TestSecurity : public QObject
{
    Q_OBJECT

private slots:
    void randomIsAvailableAndUnpredictable();
    void comparisonIsExactAndLengthSafe();
    void derivedKeysDependOnBothInputs();

    void encryptionRoundTrips();
    void refusesAWrongKeyOrATamperedBlob();
    void storeKeepsPlaintextOffDisk();
    void storeReadsFilesWrittenBeforeThereWasAKey();
    void storeFallsBackWhenThereIsNoKey();

    void generatesAUsableIdentity();
    void keepsTheSameIdentityAcrossRestarts();
    void storesThePrivateKeyOwnerOnly();
    void fingerprintIsTheHashOfTheDerCertificate();
    void replacesAnIdentityItCannotRead();

    void remembersAKeyOnlyOnceToldTo();
    void flagsAKnownNameArrivingWithANewKey();
    void treatsARenameAsTheOwnersBusiness();
    void survivesARestart();

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

void TestSecurity::encryptionRoundTrips()
{
    const QByteArray key = Crypto::randomBytes(Crypto::KeyBytes);
    const QByteArray plaintext = "the quick brown fox";

    const QByteArray sealed = Crypto::encrypt(plaintext, key);
    QVERIFY(!sealed.isEmpty());
    QVERIFY(!sealed.contains(plaintext));
    QCOMPARE(Crypto::decrypt(sealed, key), plaintext);

    // A fresh nonce every time, so the same input twice does not produce the
    // same bytes and leak that it was repeated.
    QVERIFY(Crypto::encrypt(plaintext, key) != sealed);

    // Empty input is a legitimate thing to store: an emptied history writes
    // an empty array, and refusing it would leave the old contents on disk.
    const QByteArray sealedEmpty = Crypto::encrypt(QByteArray(), key);
    QVERIFY(!sealedEmpty.isEmpty());
    QCOMPARE(Crypto::decrypt(sealedEmpty, key), QByteArray());

    QVERIFY(Crypto::encrypt(plaintext, QByteArray("short")).isEmpty());
}

void TestSecurity::refusesAWrongKeyOrATamperedBlob()
{
    const QByteArray key = Crypto::randomBytes(Crypto::KeyBytes);
    const QByteArray other = Crypto::randomBytes(Crypto::KeyBytes);
    const QByteArray sealed = Crypto::encrypt("known devices go here", key);

    QVERIFY(Crypto::decrypt(sealed, other).isEmpty());

    // Authentication is the point for the known-devices file: an attacker who
    // could edit it undetected could rewrite which key owns which name, which
    // is exactly the record the impersonation warning rests on.
    QByteArray tampered = sealed;
    tampered[tampered.size() - 1] = char(tampered.at(tampered.size() - 1) ^ 0x01);
    QVERIFY(Crypto::decrypt(tampered, key).isEmpty());

    QByteArray flippedBody = sealed;
    flippedBody[20] = char(flippedBody.at(20) ^ 0x01);
    QVERIFY(Crypto::decrypt(flippedBody, key).isEmpty());

    QVERIFY(Crypto::decrypt(QByteArray("too short"), key).isEmpty());
}

void TestSecurity::storeKeepsPlaintextOffDisk()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.path() + QStringLiteral("/secret.json");

    SecureStore::setMasterKeyForTesting(Crypto::randomBytes(Crypto::KeyBytes));
    SecureStore &store = SecureStore::instance();
    QVERIFY(store.open());
    QVERIFY(store.isEncrypting());

    const QByteArray contents = "{\"peerAlias\":\"Nico's MacBook\"}";
    QVERIFY(store.write(path, contents));
    QCOMPARE(store.read(path), contents);

    // The whole point: a copy of this file is worth nothing on its own.
    QFile raw(path);
    QVERIFY(raw.open(QIODevice::ReadOnly));
    const QByteArray onDisk = raw.readAll();
    QVERIFY(!onDisk.contains("MacBook"));
    QVERIFY(!onDisk.contains("peerAlias"));

    const QFile::Permissions permissions = QFile::permissions(path);
    QVERIFY(!(permissions & QFile::ReadGroup));
    QVERIFY(!(permissions & QFile::ReadOther));

    SecureStore::setMasterKeyForTesting(QByteArray());
}

void TestSecurity::storeReadsFilesWrittenBeforeThereWasAKey()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.path() + QStringLiteral("/legacy.json");

    const QByteArray legacy = "{\"written\":\"by an older build\"}";
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(legacy);
    file.close();

    // An install that gains a keystore must not lose its history and its
    // known devices, so a plaintext file is still read and only becomes
    // encrypted on the next write.
    SecureStore::setMasterKeyForTesting(Crypto::randomBytes(Crypto::KeyBytes));
    SecureStore &store = SecureStore::instance();
    QVERIFY(store.open());
    QCOMPARE(store.read(path), legacy);

    QVERIFY(store.write(path, legacy));
    QFile reread(path);
    QVERIFY(reread.open(QIODevice::ReadOnly));
    QVERIFY(!reread.readAll().contains("older build"));

    SecureStore::setMasterKeyForTesting(QByteArray());
}

void TestSecurity::storeFallsBackWhenThereIsNoKey()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.path() + QStringLiteral("/plain.json");

    SecureStore::setMasterKeyForTesting(QByteArray());
    SecureStore &store = SecureStore::instance();

    // No keystore is not a reason to refuse to run: a transfer app that will
    // not start because a daemon is missing has failed at its job.
    QVERIFY(!store.open());
    QVERIFY(!store.isEncrypting());
    QVERIFY(!store.lastError().isEmpty());

    const QByteArray contents = "{\"still\":\"usable\"}";
    QVERIFY(store.write(path, contents));
    QCOMPARE(store.read(path), contents);
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

void TestSecurity::remembersAKeyOnlyOnceToldTo()
{
    KnownDevices known;
    known.forgetAll();

    const QString key = QString(64, QLatin1Char('A'));

    // Nothing is recorded by merely being seen. If discovery wrote to this
    // store, an impostor could claim a name simply by announcing before the
    // device it is imitating.
    QVERIFY(!known.isKnown(key));
    QVERIFY(!known.conflicts(key, QStringLiteral("Nice Orange")));

    known.remember(key, QStringLiteral("Nice Orange"));
    QVERIFY(known.isKnown(key));
    QCOMPARE(known.aliasFor(key), QStringLiteral("Nice Orange"));
    QVERIFY(known.firstSeen(key).isValid());

    known.forgetAll();
}

void TestSecurity::flagsAKnownNameArrivingWithANewKey()
{
    KnownDevices known;
    known.forgetAll();

    const QString real = QString(64, QLatin1Char('A'));
    const QString impostor = QString(64, QLatin1Char('B'));

    known.remember(real, QStringLiteral("MacBook"));

    // The same name under a key we have never exchanged with is the shape
    // impersonation takes: the announcement carrying that name is an
    // unauthenticated multicast packet anybody can send.
    QVERIFY(known.conflicts(impostor, QStringLiteral("MacBook")));
    QCOMPARE(known.expectedFingerprint(QStringLiteral("MacBook")), real);

    // The key we do know is never a conflict, whatever it calls itself.
    QVERIFY(!known.conflicts(real, QStringLiteral("MacBook")));

    // And a name we have never seen is not suspicious, only new. Warning on
    // first contact would train people to dismiss the warning.
    QVERIFY(!known.conflicts(impostor, QStringLiteral("Somebody Else")));

    // Case is not part of the comparison: implementations differ on it.
    QVERIFY(!known.conflicts(real.toLower(), QStringLiteral("MacBook")));

    known.forgetAll();
}

void TestSecurity::treatsARenameAsTheOwnersBusiness()
{
    KnownDevices known;
    known.forgetAll();

    const QString key = QString(64, QLatin1Char('C'));
    known.remember(key, QStringLiteral("Old Name"));
    known.remember(key, QStringLiteral("New Name"));

    // The key is the identity; the name is a label its owner chose and may
    // change. Only the reverse - a name moving to another key - is a warning.
    QCOMPARE(known.aliasFor(key), QStringLiteral("New Name"));
    QVERIFY(!known.conflicts(key, QStringLiteral("New Name")));
    QCOMPARE(known.count(), 1);

    known.forgetAll();
}

void TestSecurity::survivesARestart()
{
    const QString key = QString(64, QLatin1Char('D'));

    {
        KnownDevices known;
        known.forgetAll();
        known.remember(key, QStringLiteral("Persistent"));
    }

    // A memory that did not survive a restart would make every launch a first
    // contact, which is the same as having none.
    KnownDevices reloaded;
    QVERIFY(reloaded.isKnown(key));
    QCOMPARE(reloaded.aliasFor(key), QStringLiteral("Persistent"));

    reloaded.forget(key);
    QVERIFY(!reloaded.isKnown(key));

    reloaded.forgetAll();
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
