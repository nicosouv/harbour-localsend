#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QSettings>
#include <QString>

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
    Q_PROPERTY(QString pin READ pin WRITE setPin NOTIFY pinChanged)

    Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled WRITE setNotificationsEnabled NOTIFY notificationsEnabledChanged)
    Q_PROPERTY(bool keepAwake READ keepAwake WRITE setKeepAwake NOTIFY keepAwakeChanged)
    Q_PROPERTY(bool historyEnabled READ historyEnabled WRITE setHistoryEnabled NOTIFY historyEnabledChanged)

    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
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

    QString pin() const;
    void setPin(const QString &pin);

    bool notificationsEnabled() const;
    void setNotificationsEnabled(bool enabled);

    bool keepAwake() const;
    void setKeepAwake(bool enabled);

    bool historyEnabled() const;
    void setHistoryEnabled(bool enabled);

    QString language() const;
    void setLanguage(const QString &language);

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
    void notificationsEnabledChanged();
    void keepAwakeChanged();
    void historyEnabledChanged();
    void languageChanged();

private:
    QString detectDeviceModel() const;
    QString defaultDestination() const;

    QSettings m_settings;
    QString m_deviceModel;
    QString m_fingerprint;
};

#endif // APPSETTINGS_H
