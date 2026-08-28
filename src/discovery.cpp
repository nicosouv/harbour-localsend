#include "discovery.h"

#include <QHostAddress>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUdpSocket>
#include <QUrl>

#include "appsettings.h"
#include "crypto.h"
#include "devicemodel.h"
#include "protocol.h"
#include "tlsclient.h"

namespace {

// Peers answer our announcements, so our own cadence is what keeps their
// entries fresh. Slow enough to be invisible on a battery, fast enough that
// a device that just joined shows up while somebody is still looking.
const int AnnounceIntervalMs = 20 * 1000;

// Two missed rounds plus slack. Anything quieter than this is gone.
const int DeviceMaxAgeSeconds = 70;

// The subnet sweep, sized for a phone: enough sockets in flight to finish a
// /24 in well under a minute, few enough not to swamp the Wi-Fi chip.
const int ScanWindow = 24;
const int ScanTimeoutMs = 1500;
const int RegisterTimeoutMs = 3000;

} // namespace

Discovery::Discovery(AppSettings *settings, DeviceModel *devices,
                     QNetworkAccessManager *network, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_devices(devices)
    , m_network(network)
    , m_socket(0)
    , m_announceTimer(new QTimer(this))
    , m_running(false)
    , m_scanTotal(0)
    , m_scanDone(0)
    , m_scanInFlight(0)
    , m_scanning(false)
    , m_manualPending(0)
    , m_manualFound(false)
{
    m_announceTimer->setInterval(AnnounceIntervalMs);
    connect(m_announceTimer, &QTimer::timeout, this, &Discovery::announce);
    connect(m_devices, &DeviceModel::deviceAppeared,
            this, &Discovery::deviceAppeared);
}

bool Discovery::isRunning() const { return m_running; }
bool Discovery::isScanning() const { return m_scanning; }

bool Discovery::multicastReady() const
{
    return m_running && !m_joinedInterfaces.isEmpty();
}

int Discovery::scanProgress() const
{
    if (!m_scanning || m_scanTotal <= 0)
        return 0;
    return (m_scanDone * 100) / m_scanTotal;
}

QString Discovery::localAddress() const
{
    const QStringList addresses = localAddresses();
    return addresses.isEmpty() ? QString() : addresses.first();
}

QStringList Discovery::localAddresses() const
{
    QStringList result;
    const QList<QHostAddress> all = QNetworkInterface::allAddresses();
    for (int i = 0; i < all.count(); ++i) {
        const QHostAddress &address = all.at(i);
        if (address.protocol() != QAbstractSocket::IPv4Protocol)
            continue;
        if (address.isLoopback())
            continue;
        result.append(address.toString());
    }
    return result;
}

void Discovery::start()
{
    if (m_running)
        return;

    if (!bindSocket()) {
        // Without the socket we can still be found - the HTTP server is up and
        // peers can register with us - so this is a degraded mode, not a
        // failure. The UI offers the manual sweep instead.
        m_running = true;
        m_announceTimer->start();
        emit runningChanged();
        return;
    }

    m_running = true;
    joinGroups();
    m_announceTimer->start();
    emit runningChanged();

    // A burst rather than a single packet: multicast is unreliable by design,
    // and the first seconds after launch are when somebody is watching.
    announce();
    QTimer::singleShot(700, this, SLOT(announce()));
    QTimer::singleShot(2500, this, SLOT(announce()));
}

void Discovery::stop()
{
    if (!m_running)
        return;

    m_announceTimer->stop();
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = 0;
    }
    m_joinedInterfaces.clear();
    m_running = false;

    // Without our announcements nothing refreshes these, so they would sit
    // there looking live while every one of them slowly went stale.
    m_devices->clear();

    emit runningChanged();
}

bool Discovery::bindSocket()
{
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
    }

    m_socket = new QUdpSocket(this);
    connect(m_socket, &QUdpSocket::readyRead, this, &Discovery::readDatagrams);

    // ShareAddress is what lets a second LocalSend implementation on the same
    // handset coexist with us, and what lets us restart without waiting out
    // the socket's cooldown.
    const bool bound = m_socket->bind(QHostAddress::AnyIPv4, quint16(Protocol::DefaultPort),
                                      QUdpSocket::ShareAddress
                                      | QUdpSocket::ReuseAddressHint);
    if (!bound) {
        m_socket->deleteLater();
        m_socket = 0;
        return false;
    }
    return true;
}

void Discovery::joinGroups()
{
    if (!m_socket)
        return;

    const QHostAddress group(QString::fromLatin1(Protocol::MulticastAddress));
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (int i = 0; i < interfaces.count(); ++i) {
        const QNetworkInterface &interface = interfaces.at(i);
        const QNetworkInterface::InterfaceFlags flags = interface.flags();

        if (!flags.testFlag(QNetworkInterface::IsUp)
                || !flags.testFlag(QNetworkInterface::IsRunning)
                || !flags.testFlag(QNetworkInterface::CanMulticast)
                || flags.testFlag(QNetworkInterface::IsLoopBack)) {
            continue;
        }

        // Joining twice is harmless and returns false, so the set is only an
        // optimisation - what matters is that a Wi-Fi reconnect gets picked
        // up on the next round without any network-change plumbing.
        if (m_joinedInterfaces.contains(interface.index()))
            continue;
        if (m_socket->joinMulticastGroup(group, interface))
            m_joinedInterfaces.insert(interface.index());
    }
}

void Discovery::announce()
{
    joinGroups();
    sendAnnouncement(true);
    // Before the prune, or a manual device would be dropped on the very round
    // that was about to refresh it.
    refreshManualDevices();
    m_devices->prune(DeviceMaxAgeSeconds);
}

QNetworkReply *Discovery::probe(const QString &host, int port,
                                const QString &scheme)
{
    QUrl url;
    url.setScheme(scheme);
    url.setHost(host);
    url.setPort(port);
    url.setPath(QLatin1String(Protocol::ApiPrefix)
                + QLatin1String(Protocol::PathRegister));

    const bool secure = scheme == QLatin1String("https");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    if (secure) {
        TlsClient::configure(request, m_settings->identity().certificate(),
                             m_settings->identity().privateKey());
    }

    const QByteArray body = QJsonDocument(
        m_settings->self().toRegisterBody()).toJson(QJsonDocument::Compact);

    QNetworkReply *reply = m_network->post(request, body);
    if (secure) {
        // No fingerprint to pin against: we have not met this device. The
        // certificate is accepted so the introduction can happen, and
        // acceptRegisterReply() then refuses to believe the fingerprint it
        // claims unless it is the hash of that very certificate.
        TlsClient::acceptUnknown(reply);
    }
    return reply;
}

bool Discovery::acceptRegisterReply(QNetworkReply *reply, QString *alias)
{
    if (reply->error() != QNetworkReply::NoError)
        return false;

    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    if (!document.isObject())
        return false;

    DeviceInfo peer = DeviceInfo::fromPayload(document.object());
    if (!peer.isValid() || peer.fingerprint == m_settings->fingerprint())
        return false;

    const QString observed = TlsClient::observedFingerprint(reply);
    if (!observed.isEmpty() && !Crypto::equalsFold(observed, peer.fingerprint)) {
        qWarning("localsend: %s claims fingerprint %s but presented %s",
                 qPrintable(reply->url().host()),
                 qPrintable(peer.fingerprint), qPrintable(observed));
        return false;
    }

    // The reply omits the transport details, so they come from the URL we
    // reached it on rather than from the body.
    peer.address = reply->url().host();
    peer.protocol = reply->url().scheme();
    if (peer.port <= 0)
        peer.port = reply->url().port(Protocol::DefaultPort);

    m_devices->upsert(peer);
    if (alias)
        *alias = peer.alias;
    return true;
}

void Discovery::refreshManualDevices()
{
    const QStringList endpoints = m_settings->manualDevices();

    for (int i = 0; i < endpoints.count(); ++i) {
        const QString endpoint = endpoints.at(i);
        const int colon = endpoint.lastIndexOf(QLatin1Char(':'));
        if (colon <= 0)
            continue;

        const QString host = endpoint.left(colon);
        const int port = endpoint.mid(colon + 1).toInt();
        if (host.isEmpty() || port <= 0)
            continue;

        // Both transports, because a device is free to have switched since it
        // was added and nothing would tell us.
        const QString schemes[] = { QStringLiteral("https"), QStringLiteral("http") };
        for (int s = 0; s < 2; ++s) {
            QNetworkReply *reply = probe(host, port, schemes[s]);
            connect(reply, &QNetworkReply::finished, this, [this, reply]() {
                reply->deleteLater();
                acceptRegisterReply(reply, 0);
            });
            QTimer::singleShot(RegisterTimeoutMs, reply, SLOT(abort()));
        }
    }
}

bool Discovery::isManualLookupBusy() const
{
    return m_manualPending > 0;
}

void Discovery::addDeviceAt(const QString &address, int port)
{
    if (m_manualPending > 0)
        return;

    QString host = address.trimmed();
    int targetPort = port;

    // A pasted address usually carries its own port, and asking somebody to
    // split it by hand across two fields is a needless step.
    const int colon = host.lastIndexOf(QLatin1Char(':'));
    if (colon > 0 && !host.contains(QLatin1Char('['))) {
        bool ok = false;
        const int parsed = host.mid(colon + 1).toInt(&ok);
        if (ok && parsed > 0 && parsed < 65536) {
            targetPort = parsed;
            host = host.left(colon);
        }
    }

    host = host.trimmed();
    if (host.isEmpty())
        return;
    if (targetPort <= 0 || targetPort > 65535)
        targetPort = Protocol::DefaultPort;

    m_manualEndpoint = host + QLatin1Char(':') + QString::number(targetPort);
    m_manualAlias.clear();
    m_manualFound = false;
    m_manualPending = 2;
    emit manualLookupChanged();

    const QString schemes[] = { QStringLiteral("https"), QStringLiteral("http") };
    for (int s = 0; s < 2; ++s) {
        QNetworkReply *reply = probe(host, targetPort, schemes[s]);
        connect(reply, &QNetworkReply::finished,
                this, &Discovery::onManualReplyFinished);
        QTimer::singleShot(RegisterTimeoutMs, reply, SLOT(abort()));
    }
}

void Discovery::onManualReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    --m_manualPending;

    // First success wins; the other transport's reply is then irrelevant.
    if (!m_manualFound) {
        QString alias;
        if (acceptRegisterReply(reply, &alias)) {
            m_manualFound = true;
            m_manualAlias = alias;
            // Remembered so it survives a restart and gets re-registered with
            // on every round. A device added by hand is one nothing else can
            // find, so losing it on exit would mean typing it again.
            m_settings->addManualDevice(m_manualEndpoint);
        }
    }

    if (m_manualPending > 0)
        return;

    emit manualLookupChanged();
    emit manualLookupFinished(m_manualFound, m_manualEndpoint, m_manualAlias);
}

void Discovery::refresh()
{
    if (!m_running)
        start();
    else
        announce();
}

void Discovery::sendAnnouncement(bool announcing)
{
    if (!m_socket)
        return;

    const QByteArray payload = QJsonDocument(
        m_settings->self().toAnnouncement(announcing)).toJson(QJsonDocument::Compact);
    const QHostAddress group(QString::fromLatin1(Protocol::MulticastAddress));

    // One write per interface. A phone routinely holds Wi-Fi plus a tethering
    // interface, and the socket's default outgoing interface is whichever one
    // the routing table happens to prefer - which is not always the one the
    // other device is on.
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    bool sentAny = false;

    for (int i = 0; i < interfaces.count(); ++i) {
        const QNetworkInterface &interface = interfaces.at(i);
        if (!m_joinedInterfaces.contains(interface.index()))
            continue;
        m_socket->setMulticastInterface(interface);
        if (m_socket->writeDatagram(payload, group, quint16(Protocol::DefaultPort)) > 0)
            sentAny = true;
    }

    if (!sentAny)
        m_socket->writeDatagram(payload, group, quint16(Protocol::DefaultPort));
}

void Discovery::readDatagrams()
{
    if (!m_socket)
        return;

    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_socket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;
        const qint64 read = m_socket->readDatagram(datagram.data(), datagram.size(),
                                                   &sender, &senderPort);
        if (read <= 0)
            continue;
        datagram.truncate(int(read));

        const QJsonDocument document = QJsonDocument::fromJson(datagram);
        if (!document.isObject())
            continue;

        QString address = sender.toString();
        if (address.startsWith(QLatin1String("::ffff:")))
            address = address.mid(7);

        handlePayload(document.object(), address);
    }
}

void Discovery::handlePayload(const QJsonObject &payload, const QString &address)
{
    DeviceInfo peer = DeviceInfo::fromPayload(payload);
    if (!peer.isValid())
        return;

    // Our own announcement, looped back by the multicast group. Filtering on
    // the address instead would break the moment two apps share a handset.
    if (peer.fingerprint == m_settings->fingerprint())
        return;

    peer.address = address;
    m_devices->upsert(peer);

    if (payload.value(QStringLiteral("announce")).toBool(false))
        respondTo(peer);
}

void Discovery::respondTo(const DeviceInfo &peer)
{
    if (m_socket) {
        // Unicast, straight back to the announcer's discovery port. announce
        // is false so this cannot start a reply loop.
        const QByteArray payload = QJsonDocument(
            m_settings->self().toAnnouncement(false)).toJson(QJsonDocument::Compact);
        m_socket->writeDatagram(payload, QHostAddress(peer.address),
                                quint16(Protocol::DefaultPort));
    }

    postRegister(peer);
}

void Discovery::postRegister(const DeviceInfo &peer)
{
    if (peer.address.isEmpty())
        return;

    QNetworkRequest request(QUrl(peer.apiBase() + QLatin1String(Protocol::PathRegister)));
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    TlsClient::configure(request, m_settings->identity().certificate(),
                         m_settings->identity().privateKey());

    const QByteArray body = QJsonDocument(
        m_settings->self().toRegisterBody()).toJson(QJsonDocument::Compact);

    QNetworkReply *reply = m_network->post(request, body);
    // We already know who this is meant to be - it just announced itself - so
    // the certificate has to match what it announced.
    TlsClient::pin(reply, peer.fingerprint);
    connect(reply, &QNetworkReply::finished, this, &Discovery::onRegisterFinished);
    QTimer::singleShot(RegisterTimeoutMs, reply, SLOT(abort()));
}

void Discovery::onRegisterFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
        return;

    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    if (!document.isObject())
        return;

    DeviceInfo peer = DeviceInfo::fromPayload(document.object());
    if (!peer.isValid() || peer.fingerprint == m_settings->fingerprint())
        return;

    // The response omits the transport details, so they come from the URL we
    // reached rather than from the body - including the scheme, or a device
    // we just spoke to over TLS would be recorded as a plaintext one and
    // every later request to it would fail.
    peer.address = reply->url().host();
    peer.protocol = reply->url().scheme();
    if (peer.port <= 0)
        peer.port = reply->url().port(Protocol::DefaultPort);
    m_devices->upsert(peer);
}

DeviceInfo Discovery::registerPeer(const QJsonObject &payload, const QString &address)
{
    DeviceInfo peer = DeviceInfo::fromPayload(payload);
    if (peer.isValid() && peer.fingerprint != m_settings->fingerprint()) {
        peer.address = address;
        m_devices->upsert(peer);
    }
    return m_settings->self();
}

// --- subnet sweep --------------------------------------------------------

void Discovery::scanSubnet()
{
    if (m_scanning)
        return;

    m_scanQueue.clear();

    const QStringList mine = localAddresses();
    QStringList prefixes;
    for (int i = 0; i < mine.count(); ++i) {
        const int lastDot = mine.at(i).lastIndexOf(QLatin1Char('.'));
        if (lastDot <= 0)
            continue;
        const QString prefix = mine.at(i).left(lastDot + 1);
        if (!prefixes.contains(prefix))
            prefixes.append(prefix);
    }

    // A /24 around each of our own addresses. Wider prefixes exist, but a /16
    // sweep is 65k requests, which is not a feature: it is a denial of
    // service against the person's own router.
    //
    // Each address is tried on both transports. There is no way to tell from
    // outside whether a device is running encrypted or not, and the whole
    // point of this sweep is the network where nothing announced itself, so
    // there is nothing to read the answer off.
    for (int p = 0; p < prefixes.count(); ++p) {
        for (int host = 1; host <= 254; ++host) {
            const QString address = prefixes.at(p) + QString::number(host);
            if (mine.contains(address))
                continue;
            m_scanQueue.append(QStringLiteral("https://") + address);
            m_scanQueue.append(QStringLiteral("http://") + address);
        }
    }

    if (m_scanQueue.isEmpty())
        return;

    m_scanTotal = m_scanQueue.count();
    m_scanDone = 0;
    m_scanInFlight = 0;
    m_scanning = true;
    emit scanningChanged();

    pumpScanQueue();
}

void Discovery::pumpScanQueue()
{
    while (m_scanning && m_scanInFlight < ScanWindow && !m_scanQueue.isEmpty()) {
        const QUrl target(m_scanQueue.takeFirst());

        QNetworkReply *reply = probe(target.host(), m_settings->port(),
                                     target.scheme());
        connect(reply, &QNetworkReply::finished, this, &Discovery::onScanReplyFinished);
        QTimer::singleShot(ScanTimeoutMs, reply, SLOT(abort()));
        ++m_scanInFlight;
    }

    if (m_scanning && m_scanQueue.isEmpty() && m_scanInFlight == 0)
        finishScan();
}

void Discovery::onScanReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    --m_scanInFlight;
    ++m_scanDone;

    acceptRegisterReply(reply, 0);

    // The percentage moves a lot; the UI reads it from a property that only
    // changes on this signal, so emitting per reply is exactly right.
    emit scanningChanged();
    pumpScanQueue();
}

void Discovery::cancelScan()
{
    if (!m_scanning)
        return;
    m_scanQueue.clear();
    // Replies already in flight run to their timeout and settle the counter.
    if (m_scanInFlight == 0)
        finishScan();
}

void Discovery::finishScan()
{
    m_scanning = false;
    m_scanTotal = 0;
    m_scanDone = 0;
    emit scanningChanged();
}
