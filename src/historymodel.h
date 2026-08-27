#ifndef HISTORYMODEL_H
#define HISTORYMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

// What was sent and received, newest first.
//
// Persisted as a single JSON file: the list is capped, so a whole-file rewrite
// stays cheap, and a transfer log that survives a crash is worth more than one
// that is written incrementally and can end up half-parsed.
class HistoryModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    struct Record
    {
        QString id;
        QString direction;      // "send" | "receive"
        QString peerAlias;
        QString peerDeviceType;
        QDateTime timestamp;
        QStringList fileNames;
        qint64 totalBytes;
        QString status;         // "finished" | "failed" | "cancelled"
        QString destination;    // folder for receives, empty for sends

        Record();
    };

    enum Roles {
        RecordIdRole = Qt::UserRole + 1,
        DirectionRole,
        PeerAliasRole,
        PeerDeviceTypeRole,
        TimestampRole,
        FileNamesRole,
        FileCountRole,
        TotalBytesRole,
        StatusRole,
        DestinationRole
    };

    explicit HistoryModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

    int count() const;

    void append(const Record &record);

    Q_INVOKABLE void removeAt(int row);
    Q_INVOKABLE void clear();

signals:
    void countChanged();

private:
    QString storagePath() const;
    void load();
    void save();

    QList<Record> m_records;
};

#endif // HISTORYMODEL_H
