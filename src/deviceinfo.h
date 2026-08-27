#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <QDateTime>
#include <QJsonObject>
#include <QString>

// One device on the local network, as described by the LocalSend v2 handshake
// payloads. `address` and `lastSeen` are local bookkeeping: they are how we
// reach the peer and how we age it out, and they never travel over the wire.
struct DeviceInfo
{
    QString alias;
    QString version;        // protocol version spoken by the peer
    QString deviceModel;
    QString deviceType;     // mobile | desktop | web | headless | server
    QString fingerprint;    // stable per install, also used to spot ourselves
    int port;
    QString protocol;       // http | https
    bool download;          // peer offers the reverse (pull) transfer mode

    QString address;
    QDateTime lastSeen;

    DeviceInfo();

    // A peer without a fingerprint cannot be addressed or de-duplicated, so it
    // is worth nothing to us.
    bool isValid() const;

    // "http://192.168.1.5:53317/api/localsend/v2"
    QString apiBase() const;

    // The multicast announcement carries the transport details plus `announce`;
    // the /register body carries the transport details alone; the /info and
    // /register *responses* describe an already-reachable device and drop them.
    QJsonObject toAnnouncement(bool announcing) const;
    QJsonObject toRegisterBody() const;
    QJsonObject toInfoResponse() const;

    static DeviceInfo fromPayload(const QJsonObject &object);

private:
    QJsonObject basePayload() const;
};

#endif // DEVICEINFO_H
