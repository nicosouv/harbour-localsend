#include "appsettings.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSysInfo>
#include <QUrl>

#include "crypto.h"
#include "protocol.h"

#ifndef APP_VERSION
#define APP_VERSION "0.0.0"
#endif

namespace {

const char *KeyAlias = "device/alias";
const char *KeyFingerprint = "device/fingerprint";
const char *KeyPort = "network/port";
const char *KeyDestination = "storage/destination";
const char *KeyFolderPerSender = "storage/folderPerSender";
const char *KeyReceiveEnabled = "network/receiveEnabled";
const char *KeyQuickSave = "transfer/quickSave";
const char *KeyPinEnabled = "security/pinEnabled";
const char *KeyPinSalt = "security/pinSalt";
const char *KeyPinHash = "security/pinHash";
const char *KeySecureTransport = "security/encrypt";

// Written by versions before the PIN was hashed. Read once, migrated, and
// deleted; never written again.
const char *KeyLegacyPin = "security/pin";

// A PIN is four to eight digits, so the search space is small enough that the
// work factor is most of what stands between a stolen settings file and the
// code. Roughly a tenth of a second on the slowest device we target: fast
// enough to sit on the request path, slow enough that ten thousand of them is
// not a coffee break.
const int PinIterations = 120000;
const int PinSaltBytes = 16;
const int PinHashBytes = 32;
const char *KeyNotifications = "ui/notifications";
const char *KeyKeepAwake = "transfer/keepAwake";
const char *KeyHistoryEnabled = "ui/history";
const char *KeyLanguage = "ui/language";
const char *KeyManualDevices = "network/manualDevices";

} // namespace

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral("harbour-localsend"), QStringLiteral("harbour-localsend"))
{
    m_deviceModel = detectDeviceModel();

    // Both of these are written on first launch and then never regenerated: an
    // alias that changed on every start would be unrecognisable to the person
    // on the other end, and a fingerprint that changed would make us look like
    // a brand new device every time.
    if (!m_settings.contains(QLatin1String(KeyAlias)))
        m_settings.setValue(QLatin1String(KeyAlias), Protocol::generateAlias());

    m_randomFingerprint = m_settings.value(QLatin1String(KeyFingerprint)).toString();
    if (m_randomFingerprint.isEmpty()) {
        m_randomFingerprint = Protocol::generateFingerprint();
        m_settings.setValue(QLatin1String(KeyFingerprint), m_randomFingerprint);
    }

    if (!m_settings.contains(QLatin1String(KeyDestination)))
        m_settings.setValue(QLatin1String(KeyDestination), defaultDestination());

    // An upgrade from a version that kept the PIN in the clear. Re-hash it
    // and drop the plaintext, rather than leaving it sitting there for the
    // life of the install.
    const QString legacyPin = m_settings.value(QLatin1String(KeyLegacyPin)).toString();
    if (!legacyPin.isEmpty()) {
        setPin(legacyPin);
        m_settings.remove(QLatin1String(KeyLegacyPin));
    }

    m_settings.sync();

    // The PIN hash and salt live in here, so keep the file readable by its
    // owner only. The sandbox already stops other applications reaching it;
    // this is what is left for backups and anything with the filesystem open.
    QFile::setPermissions(m_settings.fileName(),
                          QFile::ReadOwner | QFile::WriteOwner);

    if (secureTransport())
        ensureIdentity();
}

void AppSettings::ensureIdentity()
{
    if (m_identity.isValid()) {
        m_transportError.clear();
        return;
    }

    const QString directory =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (m_identity.ensure(directory)) {
        m_transportError.clear();
        return;
    }

    // Falling back to plain HTTP is better than refusing to run, but it is
    // not something to do quietly: the settings page says so, and so does the
    // main page.
    m_transportError = m_identity.lastError();
    qWarning("localsend: no TLS identity (%s), falling back to plain HTTP",
             qPrintable(m_transportError));
}

QString AppSettings::detectDeviceModel() const
{
    // /etc/hw-release names the actual handset ("Xperia 10 III"), which is far
    // more useful in someone else's device list than "Sailfish OS".
    QFile hwRelease(QStringLiteral("/etc/hw-release"));
    if (hwRelease.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!hwRelease.atEnd()) {
            const QString line = QString::fromUtf8(hwRelease.readLine()).trimmed();
            if (!line.startsWith(QLatin1String("NAME=")))
                continue;
            QString value = line.mid(5).trimmed();
            if (value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
                value = value.mid(1, value.length() - 2);
            if (!value.isEmpty())
                return value;
        }
    }

    const QString product = QSysInfo::prettyProductName();
    return product.isEmpty() ? QStringLiteral("Sailfish OS") : product;
}

QString AppSettings::defaultDestination() const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (path.isEmpty())
        path = QDir::homePath() + QStringLiteral("/Downloads");
    return path;
}

QString AppSettings::alias() const
{
    return m_settings.value(QLatin1String(KeyAlias)).toString();
}

void AppSettings::setAlias(const QString &alias)
{
    const QString trimmed = alias.trimmed();
    if (trimmed.isEmpty() || trimmed == this->alias())
        return;
    m_settings.setValue(QLatin1String(KeyAlias), trimmed);
    m_settings.sync();
    emit aliasChanged();
}

QString AppSettings::deviceModel() const
{
    return m_deviceModel;
}

QString AppSettings::fingerprint() const
{
    // Under HTTPS the protocol defines the fingerprint as the certificate's
    // hash, and a peer checks what we present against it. Announcing anything
    // else would make every encrypted transfer fail verification.
    return isEncrypted() ? m_identity.fingerprint() : m_randomFingerprint;
}

int AppSettings::port() const
{
    return m_settings.value(QLatin1String(KeyPort), Protocol::DefaultPort).toInt();
}

void AppSettings::setPort(int port)
{
    if (port < 1024 || port > 65535 || port == this->port())
        return;
    m_settings.setValue(QLatin1String(KeyPort), port);
    m_settings.sync();
    emit portChanged();
}

QString AppSettings::destination() const
{
    const QString path = m_settings.value(QLatin1String(KeyDestination)).toString();
    return path.isEmpty() ? defaultDestination() : path;
}

void AppSettings::setDestination(const QString &path)
{
    // The picker hands back a URL on some paths through the UI; normalise so
    // the receiver never has to guess which form it is holding.
    QString local = path;
    if (local.startsWith(QLatin1String("file://")))
        local = QUrl(local).toLocalFile();
    if (local.isEmpty() || local == destination())
        return;
    m_settings.setValue(QLatin1String(KeyDestination), local);
    m_settings.sync();
    emit destinationChanged();
}

bool AppSettings::folderPerSender() const
{
    return m_settings.value(QLatin1String(KeyFolderPerSender), false).toBool();
}

void AppSettings::setFolderPerSender(bool enabled)
{
    if (enabled == folderPerSender())
        return;
    m_settings.setValue(QLatin1String(KeyFolderPerSender), enabled);
    m_settings.sync();
    emit folderPerSenderChanged();
}

bool AppSettings::receiveEnabled() const
{
    return m_settings.value(QLatin1String(KeyReceiveEnabled), true).toBool();
}

void AppSettings::setReceiveEnabled(bool enabled)
{
    if (enabled == receiveEnabled())
        return;
    m_settings.setValue(QLatin1String(KeyReceiveEnabled), enabled);
    m_settings.sync();
    emit receiveEnabledChanged();
}

bool AppSettings::quickSave() const
{
    return m_settings.value(QLatin1String(KeyQuickSave), false).toBool();
}

void AppSettings::setQuickSave(bool enabled)
{
    if (enabled == quickSave())
        return;
    m_settings.setValue(QLatin1String(KeyQuickSave), enabled);
    m_settings.sync();
    emit quickSaveChanged();
}

bool AppSettings::pinEnabled() const
{
    return m_settings.value(QLatin1String(KeyPinEnabled), false).toBool();
}

void AppSettings::setPinEnabled(bool enabled)
{
    if (enabled == pinEnabled())
        return;
    m_settings.setValue(QLatin1String(KeyPinEnabled), enabled);
    m_settings.sync();
    emit pinEnabledChanged();
}

bool AppSettings::pinIsSet() const
{
    return !m_settings.value(QLatin1String(KeyPinHash)).toString().isEmpty();
}

void AppSettings::setPin(const QString &pin)
{
    if (pin.isEmpty()) {
        m_settings.remove(QLatin1String(KeyPinSalt));
        m_settings.remove(QLatin1String(KeyPinHash));
        m_settings.remove(QLatin1String(KeyLegacyPin));
        m_settings.sync();
        emit pinChanged();
        return;
    }

    const QByteArray salt = Crypto::randomBytes(PinSaltBytes);
    if (salt.isEmpty())
        return;   // no random source; storing an unsalted hash would be worse

    const QByteArray hash = Crypto::deriveKey(pin, salt, PinIterations, PinHashBytes);
    if (hash.isEmpty())
        return;

    m_settings.setValue(QLatin1String(KeyPinSalt), QString::fromLatin1(salt.toHex()));
    m_settings.setValue(QLatin1String(KeyPinHash), QString::fromLatin1(hash.toHex()));
    m_settings.remove(QLatin1String(KeyLegacyPin));
    m_settings.sync();

    QFile::setPermissions(m_settings.fileName(),
                          QFile::ReadOwner | QFile::WriteOwner);
    emit pinChanged();
}

bool AppSettings::checkPin(const QString &candidate) const
{
    if (!pinEnabled())
        return true;

    const QByteArray salt = QByteArray::fromHex(
        m_settings.value(QLatin1String(KeyPinSalt)).toString().toLatin1());
    const QByteArray expected = QByteArray::fromHex(
        m_settings.value(QLatin1String(KeyPinHash)).toString().toLatin1());

    // The requirement is switched on but no code was ever set. Refusing
    // everything would leave the device unreachable with no way for the
    // sender to tell why, so treat it as no barrier; the settings page is
    // what stops this state being reachable.
    if (salt.isEmpty() || expected.isEmpty())
        return true;

    const QByteArray derived =
            Crypto::deriveKey(candidate, salt, PinIterations, expected.size());
    if (derived.isEmpty())
        return false;

    return Crypto::equals(derived, expected);
}

bool AppSettings::secureTransport() const
{
    return m_settings.value(QLatin1String(KeySecureTransport), true).toBool();
}

void AppSettings::setSecureTransport(bool enabled)
{
    if (enabled == secureTransport())
        return;

    m_settings.setValue(QLatin1String(KeySecureTransport), enabled);
    m_settings.sync();

    if (enabled)
        ensureIdentity();

    // The fingerprint we advertise is the certificate hash under HTTPS and a
    // random value otherwise, so flipping this makes us a different device as
    // far as every peer is concerned. That is correct - it is a different
    // key - but it is why the setting is not something to toggle idly.
    emit secureTransportChanged();
}

bool AppSettings::isEncrypted() const
{
    return secureTransport() && m_identity.isValid();
}

QString AppSettings::transportError() const
{
    return m_transportError;
}

const Certificate &AppSettings::identity() const
{
    return m_identity;
}

bool AppSettings::notificationsEnabled() const
{
    return m_settings.value(QLatin1String(KeyNotifications), true).toBool();
}

void AppSettings::setNotificationsEnabled(bool enabled)
{
    if (enabled == notificationsEnabled())
        return;
    m_settings.setValue(QLatin1String(KeyNotifications), enabled);
    m_settings.sync();
    emit notificationsEnabledChanged();
}

bool AppSettings::keepAwake() const
{
    return m_settings.value(QLatin1String(KeyKeepAwake), true).toBool();
}

void AppSettings::setKeepAwake(bool enabled)
{
    if (enabled == keepAwake())
        return;
    m_settings.setValue(QLatin1String(KeyKeepAwake), enabled);
    m_settings.sync();
    emit keepAwakeChanged();
}

bool AppSettings::historyEnabled() const
{
    return m_settings.value(QLatin1String(KeyHistoryEnabled), true).toBool();
}

void AppSettings::setHistoryEnabled(bool enabled)
{
    if (enabled == historyEnabled())
        return;
    m_settings.setValue(QLatin1String(KeyHistoryEnabled), enabled);
    m_settings.sync();
    emit historyEnabledChanged();
}

QString AppSettings::language() const
{
    return m_settings.value(QLatin1String(KeyLanguage), QStringLiteral("en")).toString();
}

void AppSettings::setLanguage(const QString &language)
{
    if (language.isEmpty() || language == this->language())
        return;
    m_settings.setValue(QLatin1String(KeyLanguage), language);
    m_settings.sync();
    emit languageChanged();
}

QStringList AppSettings::manualDevices() const
{
    return m_settings.value(QLatin1String(KeyManualDevices)).toStringList();
}

void AppSettings::addManualDevice(const QString &endpoint)
{
    const QString trimmed = endpoint.trimmed();
    if (trimmed.isEmpty())
        return;

    QStringList devices = manualDevices();
    if (devices.contains(trimmed))
        return;

    devices.append(trimmed);
    m_settings.setValue(QLatin1String(KeyManualDevices), devices);
    m_settings.sync();
    emit manualDevicesChanged();
}

void AppSettings::removeManualDevice(const QString &endpoint)
{
    QStringList devices = manualDevices();
    if (devices.removeAll(endpoint) == 0)
        return;

    m_settings.setValue(QLatin1String(KeyManualDevices), devices);
    m_settings.sync();
    emit manualDevicesChanged();
}

QString AppSettings::appVersion() const
{
    return QStringLiteral(APP_VERSION);
}

QString AppSettings::protocolVersion() const
{
    return QString::fromLatin1(Protocol::Version);
}

DeviceInfo AppSettings::self() const
{
    DeviceInfo device;
    device.alias = alias();
    device.version = QString::fromLatin1(Protocol::Version);
    device.deviceModel = m_deviceModel;
    device.deviceType = QStringLiteral("mobile");
    device.fingerprint = fingerprint();
    device.port = port();
    device.protocol = isEncrypted() ? QStringLiteral("https")
                                    : QStringLiteral("http");
    device.download = false;
    return device;
}

QString AppSettings::suggestAlias() const
{
    return Protocol::generateAlias();
}

void AppSettings::rerollAlias()
{
    setAlias(Protocol::generateAlias());
}
