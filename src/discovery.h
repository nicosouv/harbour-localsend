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

    // A peer POSTed /register to us. Records it and hands back our own info
    // for the response body.
    DeviceInfo registerPeer(const QJsonObject &payload, const QString &address);

signals:
    void runningChanged();
    void scanningChanged();
    void deviceAppeared(const QString &alias);

private slots:
    void readDatagrams();
    void announce();
    void onRegisterFinished();
    void onScanReplyFinished();

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
};

#endif // DISCOVERY_H
