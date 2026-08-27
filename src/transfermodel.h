#ifndef TRANSFERMODEL_H
#define TRANSFERMODEL_H

#include <QAbstractListModel>
#include <QElapsedTimer>
#include <QList>
#include <QString>

class QTimer;

// One file inside a transfer, in either direction. `localPath` is the source
// when sending and the destination when receiving; `token` is the per-file
// secret the protocol issues in prepare-upload.
struct FileEntry
{
    QString id;
    QString fileName;
    qint64 size;
    QString fileType;
    QString sha256;
    QString preview;

    QString token;
    QString localPath;
    qint64 transferred;
    int status;
    QString error;

    FileEntry();
};

// The one transfer that can be in flight, as a list model plus the aggregate
// figures the UI reads. The protocol allows a single session at a time - a
// second one is answered with 409 - so a single instance is shared by the
// send and receive services rather than one each.
//
// Progress arrives once per socket read, which on a fast link is hundreds of
// times a second. Nothing is emitted at that rate: updates are folded into
// the entries and flushed to QML on a timer.
class TransferModel : public QAbstractListModel
{
    Q_OBJECT

    // State and direction cross to QML as strings rather than enum ints: a
    // context property carries no enum namespace with it, so the alternative
    // is magic numbers scattered through the pages.
    Q_PROPERTY(QString state READ stateName NOTIFY stateChanged)
    Q_PROPERTY(QString direction READ directionName NOTIFY stateChanged)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool sending READ sending NOTIFY stateChanged)

    Q_PROPERTY(QString peerAlias READ peerAlias NOTIFY peerChanged)
    Q_PROPERTY(QString peerAddress READ peerAddress NOTIFY peerChanged)
    Q_PROPERTY(QString peerDeviceType READ peerDeviceType NOTIFY peerChanged)

    Q_PROPERTY(int fileCount READ fileCount NOTIFY progressChanged)
    Q_PROPERTY(int completedCount READ completedCount NOTIFY progressChanged)
    Q_PROPERTY(int failedCount READ failedCount NOTIFY progressChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY progressChanged)

    Q_PROPERTY(qint64 totalBytes READ totalBytes NOTIFY progressChanged)
    Q_PROPERTY(qint64 transferredBytes READ transferredBytes NOTIFY progressChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(qint64 bytesPerSecond READ bytesPerSecond NOTIFY progressChanged)
    Q_PROPERTY(int secondsRemaining READ secondsRemaining NOTIFY progressChanged)
    Q_PROPERTY(int elapsedSeconds READ elapsedSeconds NOTIFY progressChanged)

    Q_PROPERTY(QString destination READ destination NOTIFY destinationChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY stateChanged)

public:
    enum State {
        Idle,
        Pending,     // an incoming request is waiting for the user
        Requesting,  // an outgoing request is waiting for the other device
        Active,
        Finished,
        Failed,
        Cancelled
    };

    enum Direction { NoDirection, Sending, Receiving };

    enum FileStatus {
        FileWaiting,
        FileTransferring,
        FileDone,
        FileFailed,
        FileSkipped
    };

    enum Roles {
        FileIdRole = Qt::UserRole + 1,
        FileNameRole,
        FileSizeRole,
        FileTypeRole,
        TransferredRole,
        FileProgressRole,
        FileStatusRole,
        FileErrorRole,
        LocalPathRole
    };

    explicit TransferModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

    QString stateName() const;
    QString directionName() const;
    State state() const;
    Direction direction() const;
    bool active() const;
    bool sending() const;

    QString peerAlias() const;
    QString peerAddress() const;
    QString peerDeviceType() const;

    int fileCount() const;
    int completedCount() const;
    int failedCount() const;
    int currentIndex() const;

    qint64 totalBytes() const;
    qint64 transferredBytes() const;
    qreal progress() const;
    qint64 bytesPerSecond() const;
    int secondsRemaining() const;
    int elapsedSeconds() const;

    QString destination() const;
    QString errorText() const;

    // --- driven by the send and receive services -------------------------

    void begin(Direction direction, const QString &alias, const QString &address,
               const QString &deviceType, const QList<FileEntry> &files);
    void setState(State state, const QString &error = QString());
    void setDestination(const QString &path);

    void setFileToken(int row, const QString &token);
    void setFileLocalPath(int row, const QString &path);
    void setFileStatus(int row, FileStatus status, const QString &error = QString());
    // Absolute count for this file, not a delta: the sender reports totals and
    // a dropped signal must not lose bytes.
    void setFileTransferred(int row, qint64 bytes);

    int indexOfFile(const QString &fileId) const;
    const FileEntry &entry(int row) const;
    QList<FileEntry> entries() const;

    // Everything the history needs, once the transfer is over.
    Q_INVOKABLE QStringList transferredPaths() const;

signals:
    void stateChanged();
    void peerChanged();
    void progressChanged();
    void destinationChanged();

private slots:
    void flush();

private:
    void touch(int row);
    void recomputeTotals();

    QList<FileEntry> m_files;
    State m_state;
    Direction m_direction;

    QString m_peerAlias;
    QString m_peerAddress;
    QString m_peerDeviceType;
    QString m_destination;
    QString m_errorText;

    qint64 m_totalBytes;
    qint64 m_transferredBytes;

    // The span of rows whose progress has moved since the last flush. -1 when
    // nothing is pending.
    int m_dirtyFirst;
    int m_dirtyLast;
    QTimer *m_flushTimer;

    QElapsedTimer m_clock;
    qint64 m_speedSampleMs;
    qint64 m_speedSampleBytes;
    qint64 m_bytesPerSecond;
};

#endif // TRANSFERMODEL_H
