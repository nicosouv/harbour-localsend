#include "historymodel.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace {

// Enough to answer "where did that file go", not so much that the list turns
// into an archive nobody scrolls.
const int MaxRecords = 200;

} // namespace

HistoryModel::Record::Record()
    : totalBytes(0)
{
}

HistoryModel::HistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    load();
}

QString HistoryModel::storagePath() const
{
    const QString directory =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(directory);
    return directory + QStringLiteral("/history.json");
}

void HistoryModel::load()
{
    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isArray())
        return;

    const QJsonArray array = document.array();
    for (int i = 0; i < array.count(); ++i) {
        const QJsonObject object = array.at(i).toObject();

        Record record;
        record.id = object.value(QStringLiteral("id")).toString();
        record.direction = object.value(QStringLiteral("direction")).toString();
        record.peerAlias = object.value(QStringLiteral("peerAlias")).toString();
        record.peerDeviceType = object.value(QStringLiteral("peerDeviceType")).toString();
        record.timestamp = QDateTime::fromMSecsSinceEpoch(
            qint64(object.value(QStringLiteral("timestamp")).toDouble()));
        record.totalBytes = qint64(object.value(QStringLiteral("totalBytes")).toDouble());
        record.status = object.value(QStringLiteral("status")).toString();
        record.destination = object.value(QStringLiteral("destination")).toString();

        const QJsonArray names = object.value(QStringLiteral("fileNames")).toArray();
        for (int n = 0; n < names.count(); ++n)
            record.fileNames.append(names.at(n).toString());

        m_records.append(record);
    }
}

void HistoryModel::save()
{
    QJsonArray array;
    for (int i = 0; i < m_records.count(); ++i) {
        const Record &record = m_records.at(i);

        QJsonObject object;
        object.insert(QStringLiteral("id"), record.id);
        object.insert(QStringLiteral("direction"), record.direction);
        object.insert(QStringLiteral("peerAlias"), record.peerAlias);
        object.insert(QStringLiteral("peerDeviceType"), record.peerDeviceType);
        object.insert(QStringLiteral("timestamp"),
                      double(record.timestamp.toMSecsSinceEpoch()));
        object.insert(QStringLiteral("totalBytes"), double(record.totalBytes));
        object.insert(QStringLiteral("status"), record.status);
        object.insert(QStringLiteral("destination"), record.destination);

        QJsonArray names;
        for (int n = 0; n < record.fileNames.count(); ++n)
            names.append(record.fileNames.at(n));
        object.insert(QStringLiteral("fileNames"), names);

        array.append(object);
    }

    // A crash mid-write would otherwise leave a truncated file that parses as
    // nothing, silently wiping the whole log.
    QSaveFile file(storagePath());
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(array).toJson(QJsonDocument::Compact));
    file.commit();

    QFile::setPermissions(storagePath(), QFile::ReadOwner | QFile::WriteOwner);
}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_records.count();
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_records.count())
        return QVariant();

    const Record &record = m_records.at(index.row());
    switch (role) {
    case RecordIdRole:       return record.id;
    case DirectionRole:      return record.direction;
    case PeerAliasRole:      return record.peerAlias;
    case PeerDeviceTypeRole: return record.peerDeviceType;
    case TimestampRole:      return record.timestamp;
    case FileNamesRole:      return record.fileNames;
    case FileCountRole:      return record.fileNames.count();
    case TotalBytesRole:     return record.totalBytes;
    case StatusRole:         return record.status;
    case DestinationRole:    return record.destination;
    default:                 return QVariant();
    }
}

QHash<int, QByteArray> HistoryModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[RecordIdRole] = "recordId";
    roles[DirectionRole] = "direction";
    roles[PeerAliasRole] = "peerAlias";
    roles[PeerDeviceTypeRole] = "peerDeviceType";
    roles[TimestampRole] = "timestamp";
    roles[FileNamesRole] = "fileNames";
    roles[FileCountRole] = "fileCount";
    roles[TotalBytesRole] = "totalBytes";
    roles[StatusRole] = "status";
    roles[DestinationRole] = "destination";
    return roles;
}

int HistoryModel::count() const
{
    return m_records.count();
}

void HistoryModel::append(const Record &record)
{
    beginInsertRows(QModelIndex(), 0, 0);
    m_records.prepend(record);
    endInsertRows();

    if (m_records.count() > MaxRecords) {
        const int first = MaxRecords;
        const int last = m_records.count() - 1;
        beginRemoveRows(QModelIndex(), first, last);
        while (m_records.count() > MaxRecords)
            m_records.removeLast();
        endRemoveRows();
    }

    save();
    emit countChanged();
}

void HistoryModel::removeAt(int row)
{
    if (row < 0 || row >= m_records.count())
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_records.removeAt(row);
    endRemoveRows();

    save();
    emit countChanged();
}

void HistoryModel::clear()
{
    if (m_records.isEmpty())
        return;

    beginResetModel();
    m_records.clear();
    endResetModel();

    save();
    emit countChanged();
}
