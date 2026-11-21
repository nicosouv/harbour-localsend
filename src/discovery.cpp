#include "discovery.h"

#include <QHostAddress>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTcpSocket>
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
// /24 in well under a minute, few enough not to swamp the Wi-Fi chip. The
// window is also the most descriptors a sweep ever holds, which matters
// because the process gets 1024 and the graphics driver needs some of them.
const int ScanWindow = 24;
const int ScanTimeoutMs = 1500;
const int RegisterTimeoutMs = 3000;

// Real devices announce every twenty seconds, so half of that leaves normal
// discovery untouched while capping what a flood can make us send.
const qint64 ResponseCooldownMs = 10 * 1000;
const int MaxTrackedResponders = 256;

// Announcements handled in one go before yielding to the event loop.
const int MaxDatagramsPerPass = 32;

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
    , m_scanProbes(0)
    , m_scanning(false)
    , m_scanPumping(false)
    , m_manualPending(0)
    , m_manualFound(false)
{
    m_responseClock.start();
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

    // Bounded per pass. A peer can send datagrams faster than this loop
    // retires them, and an unbounded while() would keep the event loop inside
    // discovery while the UI and any running transfer starved. Whatever is
    // left is still queued and picked up on the next readyRead.
    int budget = MaxDatagramsPerPass;

    while (m_socket->hasPendingDatagrams() && budget-- > 0) {
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
    // An announcement is an unauthenticated UDP datagram, and its source
    // address is whatever the sender wrote there. Answering every one of them
    // turns this device into a reflector: forge a thousand announcements
    // carrying somebody else's address and we send that somebody a thousand
    // HTTP requests on the attacker's behalf, from an address they never
    // touched.
    //
    // A cooldown per address is enough to make that pointless while leaving
    // ordinary discovery - one announcement every twenty seconds from each
    // real device - completely unaffected.
    const qint64 now = m_responseClock.elapsed();

    const QHash<QString, qint64>::const_iterator seen =
            m_lastResponse.constFind(peer.address);
    if (seen != m_lastResponse.constEnd()
            && now - seen.value() < ResponseCooldownMs) {
        return;
    }

    if (m_lastResponse.size() >= MaxTrackedResponders) {
        // Being flooded from many addresses at once. Forgetting the whole
        // table is the cheap way out: the cooldown restarts for everyone,
        // which costs one extra reply per real device and refuses to let the
        // table itself become the memory leak.
        m_lastResponse.clear();
    }
    m_lastResponse.insert(peer.address, now);

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

QStringList Discovery::lanAddresses() const
{
    // Only interfaces that broadcast: Wi-Fi, Ethernet, the hotspot bridge.
    // Mobile data is point-to-point, or on MediaTek modems a NOARP device
    // with no broadcast flag, and the /24 around its address belongs to the
    // operator: other subscribers or nothing at all. Sweeping it never found
    // a LocalSend device, and its unanswered connections are what ran the
    // app out of descriptors.
    QStringList result;
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (int i = 0; i < interfaces.count(); ++i) {
        const QNetworkInterface &interface = interfaces.at(i);
        const QNetworkInterface::InterfaceFlags flags = interface.flags();

        if (!flags.testFlag(QNetworkInterface::IsUp)
                || !flags.testFlag(QNetworkInterface::IsRunning)
                || !flags.testFlag(QNetworkInterface::CanBroadcast)
                || flags.testFlag(QNetworkInterface::IsLoopBack)
                || flags.testFlag(QNetworkInterface::IsPointToPoint)) {
            continue;
        }

        const QList<QNetworkAddressEntry> entries = interface.addressEntries();
        for (int e = 0; e < entries.count(); ++e) {
            const QHostAddress address = entries.at(e).ip();
            if (address.protocol() == QAbstractSocket::IPv4Protocol)
                result.append(address.toString());
        }
    }
    return result;
}

void Discovery::scanSubnet()
{
    if (m_scanning)
        return;

    const QStringList mine = lanAddresses();
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
    QStringList hosts;
    for (int p = 0; p < prefixes.count(); ++p) {
        for (int host = 1; host <= 254; ++host) {
            const QString address = prefixes.at(p) + QString::number(host);
            if (!mine.contains(address))
                hosts.append(address);
        }
    }

    scanHosts(hosts);
}

void Discovery::scanHosts(const QStringList &hosts)
{
    if (m_scanning || hosts.isEmpty())
        return;

    m_scanQueue = hosts;
    m_scanTotal = hosts.count();
    m_scanDone = 0;
    m_scanProbes = 0;
    m_scanning = true;
    emit scanningChanged();

    pumpScanQueue();
}

void Discovery::pumpScanQueue()
{
    // A refused connection can be reported from inside connectToHost(), and
    // settling it calls back in here. The outer loop fills the slot it frees.
    if (m_scanPumping)
        return;
    m_scanPumping = true;

    // A bare TCP connection first, not a request. QNetworkAccessManager keeps
    // the socket of a request that failed or was aborted open long after the
    // request is gone: one /24 left 500 descriptors behind, and Wi-Fi plus
    // mobile data took the app past its limit of 1024 in a minute. A socket
    // of our own is closed the moment it settles, and only an address that
    // answered gets a real request.
    while (m_scanning && m_scanChecks.count() < ScanWindow && !m_scanQueue.isEmpty()) {
        const QString host = m_scanQueue.takeFirst();

        QTcpSocket *socket = new QTcpSocket(this);
        m_scanChecks.insert(socket);

        connect(socket, &QTcpSocket::connected, this, [this, socket, host]() {
            finishCheck(socket, host, true);
        });
        connect(socket, static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(
                            &QAbstractSocket::error),
                this, [this, socket, host](QAbstractSocket::SocketError) {
            finishCheck(socket, host, false);
        });
        QTimer::singleShot(ScanTimeoutMs, socket, [this, socket, host]() {
            finishCheck(socket, host, false);
        });

        socket->connectToHost(host, quint16(m_settings->port()));
    }

    m_scanPumping = false;

    if (m_scanning && m_scanQueue.isEmpty() && m_scanChecks.isEmpty()
            && m_scanProbes == 0) {
        finishScan();
    }
}

void Discovery::finishCheck(QTcpSocket *socket, const QString &host, bool open)
{
    // Already settled: a timeout racing a late answer, or a cancelled sweep.
    if (!m_scanChecks.remove(socket))
        return;

    disconnect(socket, 0, this, 0);
    // abort(), so the descriptor goes back now rather than when the object
    // is finally deleted.
    socket->abort();
    socket->deleteLater();
    ++m_scanDone;

    if (open) {
        // Something listens on the LocalSend port. Both transports, because
        // nothing seen from outside says which one it serves.
        const QString schemes[] = { QStringLiteral("https"), QStringLiteral("http") };
        for (int i = 0; i < 2; ++i) {
            QNetworkReply *reply = probe(host, m_settings->port(), schemes[i]);
            connect(reply, &QNetworkReply::finished, this, &Discovery::onScanReplyFinished);
            QTimer::singleShot(RegisterTimeoutMs, reply, SLOT(abort()));
            ++m_scanProbes;
        }
    }

    // The percentage moves a lot; the UI reads it from a property that only
    // changes on this signal, so emitting per address is exactly right.
    emit scanningChanged();
    pumpScanQueue();
}

void Discovery::onScanReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();

    --m_scanProbes;
    acceptRegisterReply(reply, 0);
    pumpScanQueue();
}

void Discovery::cancelScan()
{
    if (!m_scanning)
        return;
    m_scanQueue.clear();

    // The checks are ours, so they stop now instead of at their timeout.
    const QList<QTcpSocket *> checks = m_scanChecks.values();
    m_scanChecks.clear();
    for (int i = 0; i < checks.count(); ++i) {
        disconnect(checks.at(i), 0, this, 0);
        checks.at(i)->abort();
        checks.at(i)->deleteLater();
    }

    // Probes already in flight run to their timeout and settle the counter.
    if (m_scanProbes == 0)
        finishScan();
}

void Discovery::finishScan()
{
    m_scanning = false;
    m_scanTotal = 0;
    m_scanDone = 0;
    emit scanningChanged();
}
