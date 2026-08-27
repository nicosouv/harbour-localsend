#ifndef DEVICEMODEL_H
#define DEVICEMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QVariantMap>

#include "deviceinfo.h"

class QTimer;

// The devices we can currently see. Keyed by fingerprint, because a phone that
// moves between Wi-Fi and a hotspot keeps its identity and changes its address,
// and a device list that showed both would be worse than useless.
//
// Entries are sorted by alias rather than by arrival: a list that reorders
// itself while somebody is reaching for a row is how files get sent to the
// wrong machine.
class DeviceModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        AliasRole = Qt::UserRole + 1,
        HardwareRole,          // deviceModel; never "model", which shadows the
        DeviceTypeRole,        // delegate's own model object
        FingerprintRole,
        AddressRole,
        PortRole,
        ProtocolRole,
        DownloadRole,
        LastSeenRole,
        StaleRole
    };

    explicit DeviceModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

    int count() const;

    // Adds the device or refreshes the one with the same fingerprint. Returns
    // true when this is the first time we have seen it, which is what decides
    // whether the UI should react.
    bool upsert(const DeviceInfo &device);

    void remove(const QString &fingerprint);
    void clear();

    DeviceInfo at(int row) const;
    DeviceInfo byFingerprint(const QString &fingerprint) const;
    bool contains(const QString &fingerprint) const;

    Q_INVOKABLE QVariantMap get(int row) const;

    // Drops devices that have not been heard from in a while. Called on a
    // timer by the discovery layer after each announcement round.
    void prune(int maxAgeSeconds);

signals:
    void countChanged();
    void deviceAppeared(const QString &alias);

private:
    int indexOfFingerprint(const QString &fingerprint) const;
    int insertionPoint(const DeviceInfo &device) const;

    QList<DeviceInfo> m_devices;
};

#endif // DEVICEMODEL_H
