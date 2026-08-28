#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

#include "deviceinfo.h"

class AppSettings;
class DeviceModel;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;
class QUdpSocket;

// Finds the other LocalSend devices, and makes sure they can find us.
//
// The protocol offers two ways in, and a phone needs both. Multicast is the
// fast path: announce on 224.0.0.167, and every listener answers. It is also
// the one that stops working the moment a router has client isolation on or
// a guest network in the way, and when that happens nothing tells you - the
// device list is simply empty. So there is a second path, a sweep of the
// local /24 that registers with each address in turn, offered in the UI as a
// manual action rather than run behind the user's back.
class Discovery : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(bool multicastReady READ multicastReady NOTIFY runningChanged)
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(bool manualLookupBusy READ isManualLookupBusy NOTIFY manualLookupChanged)
    Q_PROPERTY(int scanProgress READ scanProgress NOTIFY scanningChanged)
    Q_PROPERTY(QString localAddress READ localAddress NOTIFY runningChanged)

public:
    Discovery(AppSettings *settings, DeviceModel *devices,
              QNetworkAccessManager *network, QObject *parent = 0);

    bool isRunning() const;
    bool multicastReady() const;
    bool isScanning() const;
    int scanProgress() const;

    // The address peers should reach us on: the first non-loopback IPv4 we
    // hold. Shown in the UI so somebody can type it into a browser.
    QString localAddress() const;

    void start();
    void stop();

    // Announce again right now, and clear out anything that has gone quiet.
    Q_INVOKABLE void refresh();

    // Registers with every address in the local /24. The escape hatch for
    // networks that swallow multicast.
    Q_INVOKABLE void scanSubnet();
    Q_INVOKABLE void cancelScan();

    // Adds a device by address, for the case neither of the above can reach:
    // a different subnet, a VPN, a guest network with client isolation. The
    // sweep only covers our own /24, and multicast does not leave the
    // broadcast domain at all, so without this such a device is simply
    // unreachable however long you wait.
    //
    // `port` may be 0 for the default, and `address` may carry its own
    // "host:port". Both transports are tried, because there is no way to tell
    // from outside which one a device is serving.
    Q_INVOKABLE void addDeviceAt(const QString &address, int port);

    bool isManualLookupBusy() const;

    // A peer POSTed /register to us. Records it and hands back our own info
    // for the response body.
    DeviceInfo registerPeer(const QJsonObject &payload, const QString &address);

signals:
    void runningChanged();
    void scanningChanged();
    void manualLookupChanged();
    void manualLookupFinished(bool found, const QString &endpoint,
                              const QString &peerAlias);
    void deviceAppeared(const QString &alias);

private slots:
    void readDatagrams();
    void announce();
    void onRegisterFinished();
    void onScanReplyFinished();
    void onManualReplyFinished();

private:
    bool bindSocket();
    void joinGroups();
    void sendAnnouncement(bool announcing);
    // Answers a peer's announcement the way LocalSend itself does: a unicast
    // datagram for speed, and a /register POST because that is the one a
    // client behind an asymmetric firewall will actually receive.
    void respondTo(const DeviceInfo &peer);
    void postRegister(const DeviceInfo &peer);
    void handlePayload(const QJsonObject &payload, const QString &address);
    void pumpScanQueue();
    void finishScan();
    QStringList localAddresses() const;

    // Posts /register to one endpoint on one transport. The reply is left
    // unconnected for the caller to wire to whichever handler it needs.
    QNetworkReply *probe(const QString &host, int port, const QString &scheme);
    // Records a peer learned from a /register reply, refusing one whose
    // announced fingerprint is not the hash of the certificate it presented.
    bool acceptRegisterReply(QNetworkReply *reply, QString *alias);
    // Re-registers with every manually added device. Nothing else refreshes
    // them - that is the whole reason they were added by hand - so without
    // this they would be pruned as stale a minute after being found.
    void refreshManualDevices();

    AppSettings *m_settings;
    DeviceModel *m_devices;
    QNetworkAccessManager *m_network;

    QUdpSocket *m_socket;
    QTimer *m_announceTimer;
    bool m_running;
    QSet<int> m_joinedInterfaces;

    QStringList m_scanQueue;
    int m_scanTotal;
    int m_scanDone;
    int m_scanInFlight;
    bool m_scanning;

    // One manual lookup at a time, over both transports, so two replies are
    // outstanding and the first success wins.
    int m_manualPending;
    bool m_manualFound;
    QString m_manualEndpoint;
    QString m_manualAlias;
};

#endif // DISCOVERY_H
