#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QtTest>

#include "appsettings.h"
#include "certificate.h"
#include "devicemodel.h"
#include "discovery.h"
#include "historymodel.h"
#include "receiveservice.h"
#include "sendservice.h"
#include "tlsclient.h"
#include "transfermodel.h"

// Both ends of the protocol in one process, talking over the loopback.
//
// This is the test that matters: it is the only one that proves a file put in
// at one end comes out byte-identical at the other, through the real HTTP
// layer, the real handshake and the real disk writes. The unit tests around
// it exist to say *why* it broke when it does.
class TestTransfer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void sendsFilesEndToEnd();
    void sendsFilesOverTls();
    void announcesAnUppercaseFingerprint();
    void acceptsEitherFingerprintCasing();
    void refusesAPeerWhoseCertificateDoesNotMatch();
    void refusesASenderClaimingSomebodyElsesFingerprint();
    void receiverCanDeclineTheRequest();
    void manualAcceptStartsTheTransfer();
    void pinIsRequiredAndAccepted();
    void wrongPinIsRefused();
    void rejectsASecondSessionWhileBusy();
    void fileNamesCannotEscapeTheDestination();

private:
    struct Reply
    {
        int status;
        QByteArray body;
        Reply() : status(0) {}
    };

    QVariantMap loopbackDevice() const;
    QString writeFile(const QString &name, const QByteArray &content);
    Reply post(const QString &path, const QByteArray &body);
    void restartReceiver();
    static quint16 freePort();

    QTemporaryDir *m_home;
    QTemporaryDir *m_source;
    AppSettings *m_settings;
    QNetworkAccessManager *m_network;
    DeviceModel *m_devices;
    Discovery *m_discovery;
    HistoryModel *m_history;
    TransferModel *m_incoming;
    TransferModel *m_outgoing;
    ReceiveService *m_receiver;
    SendService *m_sender;
};

void TestTransfer::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

quint16 TestTransfer::freePort()
{
    // Ask the kernel for one rather than guessing: 53317 is very likely to be
    // taken by whatever the developer is testing against.
    QTcpServer probe;
    probe.listen(QHostAddress::LocalHost, 0);
    const quint16 port = probe.serverPort();
    probe.close();
    return port;
}

void TestTransfer::init()
{
    m_home = new QTemporaryDir;
    m_source = new QTemporaryDir;
    QVERIFY(m_home->isValid());
    QVERIFY(m_source->isValid());

    m_settings = new AppSettings;
    m_settings->setDestination(m_home->path());
    m_settings->setPort(freePort());
    m_settings->setQuickSave(true);
    m_settings->setPinEnabled(false);
    m_settings->setPin(QString());
    m_settings->setFolderPerSender(false);
    m_settings->setHistoryEnabled(false);
    // Plain HTTP unless a test asks otherwise, so the protocol logic is
    // exercised without TLS in the way of reading a failure.
    m_settings->setSecureTransport(false);

    m_network = new QNetworkAccessManager;
    m_devices = new DeviceModel;
    m_discovery = new Discovery(m_settings, m_devices, m_network);
    m_history = new HistoryModel;
    m_incoming = new TransferModel;
    m_outgoing = new TransferModel;

    m_receiver = new ReceiveService(m_settings, m_discovery, m_incoming,
                                    m_history, m_network);
    m_sender = new SendService(m_settings, m_outgoing, m_history, m_network);

    QVERIFY2(m_receiver->startListening(),
             qPrintable(m_receiver->listenError()));
}

void TestTransfer::cleanup()
{
    m_receiver->stopListening();

    delete m_sender;
    delete m_receiver;
    delete m_incoming;
    delete m_outgoing;
    delete m_history;
    delete m_discovery;
    delete m_devices;
    delete m_network;
    delete m_settings;
    delete m_source;
    delete m_home;
}

QVariantMap TestTransfer::loopbackDevice() const
{
    QVariantMap device;
    device.insert(QStringLiteral("alias"), QStringLiteral("Loopback"));
    device.insert(QStringLiteral("deviceType"), QStringLiteral("desktop"));
    // Both ends share one AppSettings here, so the receiver presents exactly
    // the identity the sender is told to expect - which is the arrangement
    // the pinning check is meant to accept.
    device.insert(QStringLiteral("fingerprint"), m_settings->fingerprint());
    device.insert(QStringLiteral("address"), QStringLiteral("127.0.0.1"));
    device.insert(QStringLiteral("port"), m_settings->port());
    device.insert(QStringLiteral("protocol"),
                  m_settings->isEncrypted() ? QStringLiteral("https")
                                            : QStringLiteral("http"));
    return device;
}

void TestTransfer::restartReceiver()
{
    m_receiver->stopListening();
    QVERIFY2(m_receiver->startListening(), qPrintable(m_receiver->listenError()));
}

QString TestTransfer::writeFile(const QString &name, const QByteArray &content)
{
    const QString path = m_source->path() + QLatin1Char('/') + name;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return QString();
    file.write(content);
    file.close();
    return path;
}

TestTransfer::Reply TestTransfer::post(const QString &path, const QByteArray &body)
{
    const QString scheme = m_settings->isEncrypted() ? QStringLiteral("https")
                                                     : QStringLiteral("http");
    QUrl url(QStringLiteral("%1://127.0.0.1:%2/api/localsend/v2%3")
             .arg(scheme).arg(m_settings->port()).arg(path));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    if (m_settings->isEncrypted()) {
        TlsClient::configure(request, m_settings->identity().certificate(),
                             m_settings->identity().privateKey());
    }

    QNetworkReply *reply = m_network->post(request, body);
    // This helper stands in for a hostile peer, not for our own client, so it
    // takes whatever certificate it is given.
    if (m_settings->isEncrypted())
        TlsClient::acceptUnknown(reply);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    Reply result;
    result.status = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.body = reply->readAll();
    reply->deleteLater();
    return result;
}

void TestTransfer::sendsFilesEndToEnd()
{
    const QByteArray first(64 * 1024, 'A');
    const QByteArray second = "a short one";

    QStringList paths;
    paths << writeFile(QStringLiteral("big.bin"), first)
          << writeFile(QStringLiteral("note.txt"), second);

    QSignalSpy finished(m_sender, SIGNAL(finished(QString, int)));
    m_sender->sendFiles(loopbackDevice(), paths);

    QTRY_VERIFY_WITH_TIMEOUT(finished.count() == 1, 15000);
    QCOMPARE(finished.first().at(0).toString(), QStringLiteral("finished"));
    QCOMPARE(finished.first().at(1).toInt(), 2);

    QCOMPARE(m_outgoing->stateName(), QStringLiteral("finished"));
    QCOMPARE(m_outgoing->completedCount(), 2);
    QCOMPARE(m_outgoing->transferredBytes(), m_outgoing->totalBytes());

    QCOMPARE(m_incoming->stateName(), QStringLiteral("finished"));
    QCOMPARE(m_incoming->completedCount(), 2);

    QFile landedBig(m_home->path() + QStringLiteral("/big.bin"));
    QVERIFY(landedBig.open(QIODevice::ReadOnly));
    QCOMPARE(landedBig.readAll(), first);

    QFile landedNote(m_home->path() + QStringLiteral("/note.txt"));
    QVERIFY(landedNote.open(QIODevice::ReadOnly));
    QCOMPARE(landedNote.readAll(), second);

    // Nothing may be left half-written under the temporary name.
    QVERIFY(!QFile::exists(m_home->path() + QStringLiteral("/big.bin.part")));
}

void TestTransfer::sendsFilesOverTls()
{
    m_settings->setSecureTransport(true);
    QVERIFY2(m_settings->isEncrypted(), qPrintable(m_settings->transportError()));
    restartReceiver();

    // Announcing "https" while serving plaintext, or the reverse, is a
    // failure mode with no error message on either side, so the two are
    // checked to agree before anything is sent.
    QCOMPARE(m_settings->self().protocol, QStringLiteral("https"));
    QCOMPARE(m_settings->fingerprint(), m_settings->identity().fingerprint());

    const QByteArray content(48 * 1024, 'S');
    QStringList paths;
    paths << writeFile(QStringLiteral("secret.bin"), content);

    QSignalSpy finished(m_sender, SIGNAL(finished(QString, int)));
    m_sender->sendFiles(loopbackDevice(), paths);

    QTRY_VERIFY_WITH_TIMEOUT(finished.count() == 1, 20000);
    QCOMPARE(finished.first().at(0).toString(), QStringLiteral("finished"));

    QFile landed(m_home->path() + QStringLiteral("/secret.bin"));
    QVERIFY(landed.open(QIODevice::ReadOnly));
    QCOMPARE(landed.readAll(), content);
}

void TestTransfer::announcesAnUppercaseFingerprint()
{
    // The reference implementation formats the hash with "{byte:02X}" and
    // compares what it receives against that. Announcing lowercase makes
    // every peer that pins strictly refuse us, and the symptom on their side
    // is a failed handshake with nothing to read.
    m_settings->setSecureTransport(true);
    QVERIFY(m_settings->isEncrypted());

    const QString fingerprint = m_settings->fingerprint();
    QCOMPARE(fingerprint.length(), 64);
    QCOMPARE(fingerprint, fingerprint.toUpper());
    QVERIFY(fingerprint != fingerprint.toLower());
}

void TestTransfer::acceptsEitherFingerprintCasing()
{
    // And the other direction: a peer announcing lowercase is not wrong, only
    // different, and refusing it would be our bug rather than theirs. This is
    // the case that made a real Mac unreachable while receiving from it
    // worked perfectly.
    m_settings->setSecureTransport(true);
    QVERIFY(m_settings->isEncrypted());
    restartReceiver();

    const QByteArray content = "case should not matter";
    QStringList paths;
    paths << writeFile(QStringLiteral("cased.txt"), content);

    QVariantMap device = loopbackDevice();
    device.insert(QStringLiteral("fingerprint"),
                  m_settings->fingerprint().toLower());

    QSignalSpy finished(m_sender, SIGNAL(finished(QString, int)));
    m_sender->sendFiles(device, paths);

    QTRY_VERIFY_WITH_TIMEOUT(finished.count() == 1, 20000);
    QCOMPARE(finished.first().at(0).toString(), QStringLiteral("finished"));

    QFile landed(m_home->path() + QStringLiteral("/cased.txt"));
    QVERIFY(landed.open(QIODevice::ReadOnly));
    QCOMPARE(landed.readAll(), content);
}

void TestTransfer::refusesAPeerWhoseCertificateDoesNotMatch()
{
    // The whole security model rests on this: a self-signed certificate means
    // nothing on its own, and the only thing that makes it an identity is
    // that it hashes to the fingerprint the device announced. If a mismatch
    // were accepted anyway, the encryption would be protecting a conversation
    // with whoever answered the socket.
    m_settings->setSecureTransport(true);
    QVERIFY(m_settings->isEncrypted());
    restartReceiver();

    QStringList paths;
    paths << writeFile(QStringLiteral("not-sent.bin"), "must not arrive");

    QVariantMap impostor = loopbackDevice();
    impostor.insert(QStringLiteral("fingerprint"),
                    QString(64, QLatin1Char('a')));

    QSignalSpy finished(m_sender, SIGNAL(finished(QString, int)));
    m_sender->sendFiles(impostor, paths);

    QTRY_VERIFY_WITH_TIMEOUT(finished.count() == 1, 20000);
    QCOMPARE(finished.first().at(0).toString(), QStringLiteral("failed"));
    QCOMPARE(m_outgoing->completedCount(), 0);

    // And nothing was written on the other side either: the request never got
    // as far as a session.
    QVERIFY(!QFile::exists(m_home->path() + QStringLiteral("/not-sent.bin")));
    QCOMPARE(m_incoming->stateName(), QStringLiteral("idle"));
}

void TestTransfer::refusesASenderClaimingSomebodyElsesFingerprint()
{
    // The mirror of the pinning test, on the receiving side. A sender puts its
    // fingerprint in the prepare-upload body, and that is the identity the
    // accept prompt shows. If it were taken on trust, anybody could arrive
    // wearing the name and key of a device the recipient has exchanged with
    // before, and the prompt would vouch for them.
    m_settings->setSecureTransport(true);
    QVERIFY(m_settings->isEncrypted());
    restartReceiver();

    // A second identity, standing in for a different device on the network.
    QTemporaryDir elsewhere;
    QVERIFY(elsewhere.isValid());
    Certificate impostor;
    QVERIFY2(impostor.ensure(elsewhere.path()), qPrintable(impostor.lastError()));
    QVERIFY(impostor.fingerprint() != m_settings->fingerprint());

    QJsonObject descriptor;
    descriptor.insert(QStringLiteral("id"), QStringLiteral("f"));
    descriptor.insert(QStringLiteral("fileName"), QStringLiteral("borrowed.txt"));
    descriptor.insert(QStringLiteral("size"), 4);

    QJsonObject files;
    files.insert(QStringLiteral("f"), descriptor);

    QJsonObject info;
    info.insert(QStringLiteral("alias"), QStringLiteral("Someone Trusted"));
    // Claims a key it does not hold: it will handshake as the impostor.
    info.insert(QStringLiteral("fingerprint"),
                QString(64, QLatin1Char('E')));

    QJsonObject payload;
    payload.insert(QStringLiteral("info"), info);
    payload.insert(QStringLiteral("files"), files);

    QUrl url(QStringLiteral("https://127.0.0.1:%1/api/localsend/v2/prepare-upload")
             .arg(m_settings->port()));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    TlsClient::configure(request, impostor.certificate(), impostor.privateKey());

    QNetworkReply *reply = m_network->post(
        request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    TlsClient::acceptUnknown(reply);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(8000, &loop, &QEventLoop::quit);
    loop.exec();

    const int status = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();

    QCOMPARE(status, 403);
    // And nobody was asked about it: the request never became a session.
    QCOMPARE(m_incoming->stateName(), QStringLiteral("idle"));
}

void TestTransfer::receiverCanDeclineTheRequest()
{
    m_settings->setQuickSave(false);

    QStringList paths;
    paths << writeFile(QStringLiteral("offer.txt"), "please take this");

    QSignalSpy arrived(m_receiver, SIGNAL(requestArrived(QString, int, qint64)));
    QSignalSpy declined(m_sender, SIGNAL(declined()));

    m_sender->sendFiles(loopbackDevice(), paths);

    QTRY_VERIFY_WITH_TIMEOUT(arrived.count() == 1, 10000);
    QCOMPARE(arrived.first().at(1).toInt(), 1);
    QCOMPARE(m_incoming->stateName(), QStringLiteral("pending"));

    m_receiver->decline();

    QTRY_VERIFY_WITH_TIMEOUT(declined.count() == 1, 10000);
    QCOMPARE(m_outgoing->stateName(), QStringLiteral("cancelled"));
    QVERIFY(!QFile::exists(m_home->path() + QStringLiteral("/offer.txt")));
}

void TestTransfer::manualAcceptStartsTheTransfer()
{
    m_settings->setQuickSave(false);

    const QByteArray content = "accepted by hand";
    QStringList paths;
    paths << writeFile(QStringLiteral("manual.txt"), content);

    QSignalSpy arrived(m_receiver, SIGNAL(requestArrived(QString, int, qint64)));
    QSignalSpy finished(m_sender, SIGNAL(finished(QString, int)));

    m_sender->sendFiles(loopbackDevice(), paths);
    QTRY_VERIFY_WITH_TIMEOUT(arrived.count() == 1, 10000);

    // The parked connection has to have survived the wait, which is the whole
    // reason prepare-upload is answered late rather than early.
    m_receiver->accept();

    QTRY_VERIFY_WITH_TIMEOUT(finished.count() == 1, 15000);
    QCOMPARE(finished.first().at(0).toString(), QStringLiteral("finished"));

    QFile landed(m_home->path() + QStringLiteral("/manual.txt"));
    QVERIFY(landed.open(QIODevice::ReadOnly));
    QCOMPARE(landed.readAll(), content);
}

void TestTransfer::pinIsRequiredAndAccepted()
{
    m_settings->setPinEnabled(true);
    m_settings->setPin(QStringLiteral("4269"));

    const QByteArray content = "unlocked";
    QStringList paths;
    paths << writeFile(QStringLiteral("locked.txt"), content);

    QSignalSpy pinRequired(m_sender, SIGNAL(pinRequired(bool)));
    QSignalSpy finished(m_sender, SIGNAL(finished(QString, int)));

    m_sender->sendFiles(loopbackDevice(), paths);

    QTRY_VERIFY_WITH_TIMEOUT(pinRequired.count() == 1, 10000);
    // First ask, so the UI must not accuse anybody of getting it wrong yet.
    QCOMPARE(pinRequired.first().at(0).toBool(), false);

    m_sender->submitPin(QStringLiteral("4269"));

    QTRY_VERIFY_WITH_TIMEOUT(finished.count() == 1, 15000);
    QCOMPARE(finished.first().at(0).toString(), QStringLiteral("finished"));

    QFile landed(m_home->path() + QStringLiteral("/locked.txt"));
    QVERIFY(landed.open(QIODevice::ReadOnly));
    QCOMPARE(landed.readAll(), content);
}

void TestTransfer::wrongPinIsRefused()
{
    m_settings->setPinEnabled(true);
    m_settings->setPin(QStringLiteral("1111"));

    QStringList paths;
    paths << writeFile(QStringLiteral("nope.txt"), "should not land");

    QSignalSpy pinRequired(m_sender, SIGNAL(pinRequired(bool)));
    m_sender->sendFiles(loopbackDevice(), paths);

    QTRY_VERIFY_WITH_TIMEOUT(pinRequired.count() == 1, 10000);

    m_sender->submitPin(QStringLiteral("2222"));

    QTRY_VERIFY_WITH_TIMEOUT(pinRequired.count() == 2, 10000);
    // Second ask: this one was a wrong guess, and the UI should say so.
    QCOMPARE(pinRequired.at(1).at(0).toBool(), true);
    QVERIFY(!QFile::exists(m_home->path() + QStringLiteral("/nope.txt")));
}

void TestTransfer::rejectsASecondSessionWhileBusy()
{
    m_settings->setQuickSave(false);

    QStringList paths;
    paths << writeFile(QStringLiteral("first.txt"), "one at a time");

    QSignalSpy arrived(m_receiver, SIGNAL(requestArrived(QString, int, qint64)));
    m_sender->sendFiles(loopbackDevice(), paths);
    QTRY_VERIFY_WITH_TIMEOUT(arrived.count() == 1, 10000);

    // A second sender arriving mid-decision would otherwise overwrite the
    // first one's tokens, and both transfers would fail in confusing ways.
    QJsonObject descriptor;
    descriptor.insert(QStringLiteral("id"), QStringLiteral("x"));
    descriptor.insert(QStringLiteral("fileName"), QStringLiteral("intruder.txt"));
    descriptor.insert(QStringLiteral("size"), 5);

    QJsonObject files;
    files.insert(QStringLiteral("x"), descriptor);

    QJsonObject info;
    info.insert(QStringLiteral("alias"), QStringLiteral("Intruder"));
    info.insert(QStringLiteral("fingerprint"), QStringLiteral("intruder"));

    QJsonObject payload;
    payload.insert(QStringLiteral("info"), info);
    payload.insert(QStringLiteral("files"), files);

    const Reply reply = post(QStringLiteral("/prepare-upload"),
                             QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QCOMPARE(reply.status, 409);

    m_receiver->decline();
}

void TestTransfer::fileNamesCannotEscapeTheDestination()
{
    // The sender chooses this string. Nothing stops a hostile one from
    // choosing a path that walks out of the download folder, so the receiver
    // has to be the thing that refuses.
    QJsonObject descriptor;
    descriptor.insert(QStringLiteral("id"), QStringLiteral("evil"));
    descriptor.insert(QStringLiteral("fileName"),
                      QStringLiteral("../../escaped.txt"));
    descriptor.insert(QStringLiteral("size"), 5);

    QJsonObject files;
    files.insert(QStringLiteral("evil"), descriptor);

    QJsonObject info;
    info.insert(QStringLiteral("alias"), QStringLiteral("Hostile"));
    info.insert(QStringLiteral("fingerprint"), QStringLiteral("hostile"));

    QJsonObject payload;
    payload.insert(QStringLiteral("info"), info);
    payload.insert(QStringLiteral("files"), files);

    const Reply prepared = post(QStringLiteral("/prepare-upload"),
                                QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QCOMPARE(prepared.status, 200);

    const QJsonObject response = QJsonDocument::fromJson(prepared.body).object();
    const QString sessionId = response.value(QStringLiteral("sessionId")).toString();
    const QString token = response.value(QStringLiteral("files")).toObject()
                          .value(QStringLiteral("evil")).toString();
    QVERIFY(!sessionId.isEmpty());
    QVERIFY(!token.isEmpty());

    const Reply uploaded = post(
        QStringLiteral("/upload?sessionId=%1&fileId=evil&token=%2")
            .arg(sessionId).arg(token),
        QByteArray("proof"));
    QCOMPARE(uploaded.status, 200);

    // The directory part is dropped, so the file lands inside the destination
    // under its bare name and nowhere else.
    QVERIFY(QFile::exists(m_home->path() + QStringLiteral("/escaped.txt")));

    const QDir above(m_home->path() + QStringLiteral("/../.."));
    QVERIFY(!QFile::exists(above.absolutePath() + QStringLiteral("/escaped.txt")));
}

QTEST_MAIN(TestTransfer)
#include "tst_transfer.moc"
