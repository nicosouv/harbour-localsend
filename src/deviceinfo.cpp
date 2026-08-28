#include "deviceinfo.h"

#include "protocol.h"

DeviceInfo::DeviceInfo()
    : port(Protocol::DefaultPort)
    , protocol(QStringLiteral("http"))
    , download(false)
{
}

bool DeviceInfo::isValid() const
{
    return !fingerprint.isEmpty();
}

QString DeviceInfo::apiBase() const
{
    return QString::fromLatin1("%1://%2:%3%4")
            .arg(protocol.isEmpty() ? QStringLiteral("http") : protocol)
            .arg(address)
            .arg(port)
            .arg(QLatin1String(Protocol::ApiPrefix));
}

QJsonObject DeviceInfo::basePayload() const
{
    QJsonObject object;
    object.insert(QStringLiteral("alias"), alias);
    object.insert(QStringLiteral("version"), version);
    object.insert(QStringLiteral("deviceModel"), deviceModel);
    object.insert(QStringLiteral("deviceType"), deviceType);
    object.insert(QStringLiteral("fingerprint"), fingerprint);
    object.insert(QStringLiteral("download"), download);
    return object;
}

QJsonObject DeviceInfo::toAnnouncement(bool announcing) const
{
    QJsonObject object = toRegisterBody();
    object.insert(QStringLiteral("announce"), announcing);
    return object;
}

QJsonObject DeviceInfo::toRegisterBody() const
{
    QJsonObject object = basePayload();
    object.insert(QStringLiteral("port"), port);
    object.insert(QStringLiteral("protocol"), protocol);
    return object;
}

QJsonObject DeviceInfo::toInfoResponse() const
{
    // The spec omits the transport details here, but our own subnet scan reads
    // them back from /info and extra keys are ignored by every other client.
    return toRegisterBody();
}

DeviceInfo DeviceInfo::fromPayload(const QJsonObject &object)
{
    // Every string here is chosen by whoever sent the payload and ends up on
    // screen, so none of it arrives unfiltered. The caps are generous next to
    // any real device name and far below what it would take to bloat the
    // device list.
    DeviceInfo device;
    device.alias = Protocol::sanitizeText(
        object.value(QStringLiteral("alias")).toString(), 64);
    device.version = Protocol::sanitizeText(
        object.value(QStringLiteral("version")).toString(), 16);
    device.deviceModel = Protocol::sanitizeText(
        object.value(QStringLiteral("deviceModel")).toString(), 64);
    device.deviceType = Protocol::sanitizeText(
        object.value(QStringLiteral("deviceType")).toString(), 16);
    device.fingerprint = Protocol::sanitizeText(
        object.value(QStringLiteral("fingerprint")).toString(), 128);
    device.download = object.value(QStringLiteral("download")).toBool(false);

    const QJsonValue portValue = object.value(QStringLiteral("port"));
    device.port = portValue.isDouble() ? portValue.toInt(Protocol::DefaultPort)
                                       : Protocol::DefaultPort;

    const QString protocol = object.value(QStringLiteral("protocol")).toString();
    device.protocol = (protocol == QLatin1String("https")) ? protocol
                                                           : QStringLiteral("http");

    if (device.alias.isEmpty())
        device.alias = QStringLiteral("Unknown device");
    if (device.deviceType.isEmpty())
        device.deviceType = QStringLiteral("desktop");

    return device;
}
