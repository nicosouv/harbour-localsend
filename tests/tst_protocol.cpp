#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

#include "deviceinfo.h"
#include "protocol.h"

// The wire format. Everything here is a claim about bytes another
// implementation will read, so the assertions name the exact keys rather than
// round-tripping through our own code and proving nothing.
class TestProtocol : public QObject
{
    Q_OBJECT

private slots:
    void parsesAFullAnnouncement();
    void fillsInMissingFields();
    void rejectsAPayloadWithNoFingerprint();
    void announcementCarriesTransportAndFlag();
    void infoResponseIsParseableByUs();
    void buildsTheApiBase();
    void fingerprintIsUniqueAndHex();
    void aliasLooksLikeALocalSendAlias();
};

void TestProtocol::parsesAFullAnnouncement()
{
    const QByteArray raw =
        "{\"alias\":\"Nice Orange\",\"version\":\"2.0\",\"deviceModel\":\"Samsung\","
        "\"deviceType\":\"mobile\",\"fingerprint\":\"abc123\",\"port\":53317,"
        "\"protocol\":\"https\",\"download\":true,\"announce\":true}";

    const DeviceInfo device =
        DeviceInfo::fromPayload(QJsonDocument::fromJson(raw).object());

    QCOMPARE(device.alias, QStringLiteral("Nice Orange"));
    QCOMPARE(device.version, QStringLiteral("2.0"));
    QCOMPARE(device.deviceModel, QStringLiteral("Samsung"));
    QCOMPARE(device.deviceType, QStringLiteral("mobile"));
    QCOMPARE(device.fingerprint, QStringLiteral("abc123"));
    QCOMPARE(device.port, 53317);
    QCOMPARE(device.protocol, QStringLiteral("https"));
    QCOMPARE(device.download, true);
}

void TestProtocol::fillsInMissingFields()
{
    // A peer that sends only a fingerprint is still addressable, so it has to
    // come out of here usable rather than blank.
    const DeviceInfo device = DeviceInfo::fromPayload(
        QJsonDocument::fromJson("{\"fingerprint\":\"f1\"}").object());

    QVERIFY(device.isValid());
    QCOMPARE(device.port, Protocol::DefaultPort);
    QCOMPARE(device.protocol, QStringLiteral("http"));
    QCOMPARE(device.download, false);
    QVERIFY(!device.alias.isEmpty());
    QVERIFY(!device.deviceType.isEmpty());

    // An unknown transport must never be echoed back into a URL scheme.
    const DeviceInfo odd = DeviceInfo::fromPayload(
        QJsonDocument::fromJson("{\"fingerprint\":\"f2\",\"protocol\":\"gopher\"}").object());
    QCOMPARE(odd.protocol, QStringLiteral("http"));
}

void TestProtocol::rejectsAPayloadWithNoFingerprint()
{
    const DeviceInfo device = DeviceInfo::fromPayload(
        QJsonDocument::fromJson("{\"alias\":\"Ghost\"}").object());
    QVERIFY(!device.isValid());
}

void TestProtocol::announcementCarriesTransportAndFlag()
{
    DeviceInfo device;
    device.alias = QStringLiteral("Sailfish");
    device.version = QStringLiteral("2.1");
    device.deviceModel = QStringLiteral("Xperia 10 III");
    device.deviceType = QStringLiteral("mobile");
    device.fingerprint = QStringLiteral("deadbeef");
    device.port = 53317;

    const QJsonObject announcing = device.toAnnouncement(true);
    QCOMPARE(announcing.value(QStringLiteral("announce")).toBool(), true);
    QCOMPARE(announcing.value(QStringLiteral("port")).toInt(), 53317);
    QCOMPARE(announcing.value(QStringLiteral("protocol")).toString(),
             QStringLiteral("http"));

    const QJsonObject responding = device.toAnnouncement(false);
    QCOMPARE(responding.value(QStringLiteral("announce")).toBool(), false);

    // A reply carrying announce:true is how a discovery storm starts.
    QVERIFY(responding.contains(QStringLiteral("announce")));

    const QJsonObject registration = device.toRegisterBody();
    QVERIFY(!registration.contains(QStringLiteral("announce")));
    QCOMPARE(registration.value(QStringLiteral("fingerprint")).toString(),
             QStringLiteral("deadbeef"));
}

void TestProtocol::infoResponseIsParseableByUs()
{
    DeviceInfo device;
    device.alias = QStringLiteral("Quiet Lemon");
    device.fingerprint = QStringLiteral("cafe");
    device.deviceType = QStringLiteral("mobile");
    device.port = 12345;

    const DeviceInfo parsed = DeviceInfo::fromPayload(device.toInfoResponse());
    QCOMPARE(parsed.alias, device.alias);
    QCOMPARE(parsed.fingerprint, device.fingerprint);
    QCOMPARE(parsed.port, 12345);
}

void TestProtocol::buildsTheApiBase()
{
    DeviceInfo device;
    device.fingerprint = QStringLiteral("x");
    device.address = QStringLiteral("192.168.1.42");
    device.port = 53317;
    device.protocol = QStringLiteral("http");

    QCOMPARE(device.apiBase(),
             QStringLiteral("http://192.168.1.42:53317/api/localsend/v2"));

    device.protocol = QStringLiteral("https");
    QCOMPARE(device.apiBase(),
             QStringLiteral("https://192.168.1.42:53317/api/localsend/v2"));
}

void TestProtocol::fingerprintIsUniqueAndHex()
{
    const QString first = Protocol::generateFingerprint();
    const QString second = Protocol::generateFingerprint();

    QCOMPARE(first.length(), 64);
    QVERIFY(first != second);

    // Uppercase, matching what the reference implementation announces and
    // compares against. A lowercase fingerprint is refused by peers that pin
    // strictly, and the failure shows up on their side as a dead handshake.
    for (int i = 0; i < first.length(); ++i) {
        const QChar character = first.at(i);
        QVERIFY(character.isDigit()
                || (character >= QLatin1Char('A') && character <= QLatin1Char('F')));
    }

    QVERIFY(!Protocol::generateToken().isEmpty());
    QVERIFY(Protocol::generateToken() != Protocol::generateToken());
}

void TestProtocol::aliasLooksLikeALocalSendAlias()
{
    const QString alias = Protocol::generateAlias();
    QCOMPARE(alias.split(QLatin1Char(' ')).count(), 2);
    QVERIFY(!alias.trimmed().isEmpty());

    QVERIFY(Protocol::isKnownDeviceType(QStringLiteral("mobile")));
    QVERIFY(Protocol::isKnownDeviceType(QStringLiteral("headless")));
    QVERIFY(!Protocol::isKnownDeviceType(QStringLiteral("toaster")));
}

QTEST_MAIN(TestProtocol)
#include "tst_protocol.moc"
