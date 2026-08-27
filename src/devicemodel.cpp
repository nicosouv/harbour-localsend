#include "devicemodel.h"

#include <QDateTime>

namespace {

// A device is drawn dimmed once it has been quiet this long, and dropped
// entirely by prune(). Two announcement rounds of slack, so a single lost
// multicast packet never makes a device blink out.
const int StaleAfterSeconds = 50;

} // namespace

DeviceModel::DeviceModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int DeviceModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_devices.count();
}

QVariant DeviceModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_devices.count())
        return QVariant();

    const DeviceInfo &device = m_devices.at(index.row());
    switch (role) {
    case AliasRole:       return device.alias;
    case HardwareRole:    return device.deviceModel;
    case DeviceTypeRole:  return device.deviceType;
    case FingerprintRole: return device.fingerprint;
    case AddressRole:     return device.address;
    case PortRole:        return device.port;
    case ProtocolRole:    return device.protocol;
    case DownloadRole:    return device.download;
    case LastSeenRole:    return device.lastSeen;
    case StaleRole:
        return device.lastSeen.isValid()
            && device.lastSeen.secsTo(QDateTime::currentDateTime()) > StaleAfterSeconds;
    default:              return QVariant();
    }
}

QHash<int, QByteArray> DeviceModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[AliasRole] = "alias";
    roles[HardwareRole] = "hardware";
    roles[DeviceTypeRole] = "deviceType";
    roles[FingerprintRole] = "fingerprint";
    roles[AddressRole] = "address";
    roles[PortRole] = "port";
    roles[ProtocolRole] = "protocol";
    roles[DownloadRole] = "download";
    roles[LastSeenRole] = "lastSeen";
    roles[StaleRole] = "stale";
    return roles;
}

int DeviceModel::count() const
{
    return m_devices.count();
}

int DeviceModel::indexOfFingerprint(const QString &fingerprint) const
{
    for (int i = 0; i < m_devices.count(); ++i) {
        if (m_devices.at(i).fingerprint == fingerprint)
            return i;
    }
    return -1;
}

int DeviceModel::insertionPoint(const DeviceInfo &device) const
{
    for (int i = 0; i < m_devices.count(); ++i) {
        const int order = QString::compare(m_devices.at(i).alias, device.alias,
                                           Qt::CaseInsensitive);
        if (order > 0)
            return i;
        // Same alias on two devices is common enough (two stock installs), so
        // the fingerprint breaks the tie and keeps the order deterministic.
        if (order == 0 && m_devices.at(i).fingerprint > device.fingerprint)
            return i;
    }
    return m_devices.count();
}

bool DeviceModel::upsert(const DeviceInfo &device)
{
    if (!device.isValid() || device.address.isEmpty())
        return false;

    DeviceInfo updated = device;
    updated.lastSeen = QDateTime::currentDateTime();

    const int existing = indexOfFingerprint(device.fingerprint);
    if (existing >= 0) {
        const DeviceInfo previous = m_devices.at(existing);
        // The alias is the sort key, so a rename has to move the row rather
        // than just repaint it.
        if (previous.alias != updated.alias) {
            beginRemoveRows(QModelIndex(), existing, existing);
            m_devices.removeAt(existing);
            endRemoveRows();

            const int target = insertionPoint(updated);
            beginInsertRows(QModelIndex(), target, target);
            m_devices.insert(target, updated);
            endInsertRows();
        } else {
            m_devices[existing] = updated;
            const QModelIndex changed = index(existing, 0);
            emit dataChanged(changed, changed);
        }
        return false;
    }

    const int target = insertionPoint(updated);
    beginInsertRows(QModelIndex(), target, target);
    m_devices.insert(target, updated);
    endInsertRows();

    emit countChanged();
    emit deviceAppeared(updated.alias);
    return true;
}

void DeviceModel::remove(const QString &fingerprint)
{
    const int row = indexOfFingerprint(fingerprint);
    if (row < 0)
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_devices.removeAt(row);
    endRemoveRows();
    emit countChanged();
}

void DeviceModel::clear()
{
    if (m_devices.isEmpty())
        return;

    beginResetModel();
    m_devices.clear();
    endResetModel();
    emit countChanged();
}

DeviceInfo DeviceModel::at(int row) const
{
    if (row < 0 || row >= m_devices.count())
        return DeviceInfo();
    return m_devices.at(row);
}

DeviceInfo DeviceModel::byFingerprint(const QString &fingerprint) const
{
    const int row = indexOfFingerprint(fingerprint);
    return row < 0 ? DeviceInfo() : m_devices.at(row);
}

bool DeviceModel::contains(const QString &fingerprint) const
{
    return indexOfFingerprint(fingerprint) >= 0;
}

QVariantMap DeviceModel::get(int row) const
{
    QVariantMap map;
    if (row < 0 || row >= m_devices.count())
        return map;

    const DeviceInfo &device = m_devices.at(row);
    map.insert(QStringLiteral("alias"), device.alias);
    map.insert(QStringLiteral("hardware"), device.deviceModel);
    map.insert(QStringLiteral("deviceType"), device.deviceType);
    map.insert(QStringLiteral("fingerprint"), device.fingerprint);
    map.insert(QStringLiteral("address"), device.address);
    map.insert(QStringLiteral("port"), device.port);
    map.insert(QStringLiteral("protocol"), device.protocol);
    map.insert(QStringLiteral("download"), device.download);
    return map;
}

void DeviceModel::prune(int maxAgeSeconds)
{
    const QDateTime now = QDateTime::currentDateTime();
    bool removed = false;

    for (int i = m_devices.count() - 1; i >= 0; --i) {
        const QDateTime seen = m_devices.at(i).lastSeen;
        if (seen.isValid() && seen.secsTo(now) <= maxAgeSeconds)
            continue;

        beginRemoveRows(QModelIndex(), i, i);
        m_devices.removeAt(i);
        endRemoveRows();
        removed = true;
    }

    if (removed)
        emit countChanged();
}
