#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>

#include "certificate.h"
#include "deviceinfo.h"

// Every persisted preference, plus the identity this device presents on the
// network. The identity is the interesting part: `fingerprint` is written once
// on first launch and must survive forever, because peers use it to recognise
// us and to filter our own announcements back out.
class AppSettings : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString alias READ alias WRITE setAlias NOTIFY aliasChanged)
    Q_PROPERTY(QString deviceModel READ deviceModel CONSTANT)
    Q_PROPERTY(QString fingerprint READ fingerprint CONSTANT)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)

    Q_PROPERTY(QString destination READ destination WRITE setDestination NOTIFY destinationChanged)
    Q_PROPERTY(bool folderPerSender READ folderPerSender WRITE setFolderPerSender NOTIFY folderPerSenderChanged)

    Q_PROPERTY(bool receiveEnabled READ receiveEnabled WRITE setReceiveEnabled NOTIFY receiveEnabledChanged)
    Q_PROPERTY(bool quickSave READ quickSave WRITE setQuickSave NOTIFY quickSaveChanged)
    Q_PROPERTY(bool pinEnabled READ pinEnabled WRITE setPinEnabled NOTIFY pinEnabledChanged)
    // There is no way to read the PIN back, by design: only whether one has
    // been set. It is stored as a salted hash, so nothing here could return
    // it even if a page asked.
    Q_PROPERTY(bool pinIsSet READ pinIsSet NOTIFY pinChanged)

    // What the user asked for, and what we actually got. They differ when
    // this device could not produce a TLS identity, which is worth surfacing
    // rather than silently downgrading.
    Q_PROPERTY(bool secureTransport READ secureTransport WRITE setSecureTransport NOTIFY secureTransportChanged)
    Q_PROPERTY(bool encrypted READ isEncrypted NOTIFY secureTransportChanged)
    Q_PROPERTY(QString transportError READ transportError NOTIFY secureTransportChanged)

    Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled WRITE setNotificationsEnabled NOTIFY notificationsEnabledChanged)
    Q_PROPERTY(bool keepAwake READ keepAwake WRITE setKeepAwake NOTIFY keepAwakeChanged)
    Q_PROPERTY(bool historyEnabled READ historyEnabled WRITE setHistoryEnabled NOTIFY historyEnabledChanged)

    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    // Bound directly as a list model on the add-by-address page.
    Q_PROPERTY(QStringList manualDevices READ manualDevices NOTIFY manualDevicesChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString protocolVersion READ protocolVersion CONSTANT)

public:
    explicit AppSettings(QObject *parent = 0);

    QString alias() const;
    void setAlias(const QString &alias);

    QString deviceModel() const;
    QString fingerprint() const;

    int port() const;
    void setPort(int port);

    QString destination() const;
    void setDestination(const QString &path);

    bool folderPerSender() const;
    void setFolderPerSender(bool enabled);

    bool receiveEnabled() const;
    void setReceiveEnabled(bool enabled);

    bool quickSave() const;
    void setQuickSave(bool enabled);

    bool pinEnabled() const;
    void setPinEnabled(bool enabled);

    bool pinIsSet() const;
    // Hashes and stores `pin`, or clears it when empty. Never keeps the
    // plaintext, and removes any left behind by an older version. Invokable
    // because there is no matching getter for QML to bind a property to.
    Q_INVOKABLE void setPin(const QString &pin);

    bool secureTransport() const;
    void setSecureTransport(bool enabled);
    bool isEncrypted() const;
    QString transportError() const;

    // The TLS identity, valid only while isEncrypted().
    const Certificate &identity() const;

    bool notificationsEnabled() const;
    void setNotificationsEnabled(bool enabled);

    bool keepAwake() const;
    void setKeepAwake(bool enabled);

    bool historyEnabled() const;
    void setHistoryEnabled(bool enabled);

    QString language() const;
    void setLanguage(const QString &language);

    // Devices added by address because nothing announces them: a different
    // subnet, a VPN, a guest network with client isolation on. Stored as
    // "host:port" and re-registered with on every announcement round, which
    // is what keeps them from being pruned as stale.
    QStringList manualDevices() const;
    Q_INVOKABLE void addManualDevice(const QString &endpoint);
    Q_INVOKABLE void removeManualDevice(const QString &endpoint);

    QString appVersion() const;
    QString protocolVersion() const;

    // The identity we put in every announcement, /info reply and prepare-upload
    // body. `address` is left empty: only the receiver of a payload knows which
    // of our interfaces it arrived on.
    DeviceInfo self() const;

    // Suggest a fresh random alias without committing it, so the settings page
    // can offer a dice roll.
    Q_INVOKABLE QString suggestAlias() const;

    // Reset the alias to a freshly generated one and persist it.
    Q_INVOKABLE void rerollAlias();

    // True when `candidate` unlocks an incoming transfer. Always true when the
    // PIN is off, so callers do not have to special-case it.
    bool checkPin(const QString &candidate) const;

signals:
    void aliasChanged();
    void portChanged();
    void destinationChanged();
    void folderPerSenderChanged();
    void receiveEnabledChanged();
    void quickSaveChanged();
    void pinEnabledChanged();
    void pinChanged();
    void secureTransportChanged();
    void notificationsEnabledChanged();
    void keepAwakeChanged();
    void historyEnabledChanged();
    void languageChanged();
    void manualDevicesChanged();

private:
    QString detectDeviceModel() const;
    QString defaultDestination() const;

    void ensureIdentity();

    QSettings m_settings;
    QString m_deviceModel;
    // Used only in plain-HTTP mode; under HTTPS the fingerprint has to be the
    // certificate's hash or no peer could verify us.
    QString m_randomFingerprint;
    Certificate m_identity;
    QString m_transportError;
};

#endif // APPSETTINGS_H
