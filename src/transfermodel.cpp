#include "transfermodel.h"

#include <QStringList>
#include <QTimer>

namespace {

// Progress arrives far faster than a screen can show it. Everything is folded
// into the entries and pushed to QML on this cadence instead.
const int FlushIntervalMs = 200;

// The speed readout is smoothed over roughly a second of samples, so it does
// not swing wildly between two consecutive socket reads.
const qint64 SpeedWindowMs = 1000;

QString fileStatusName(int status)
{
    switch (status) {
    case TransferModel::FileTransferring: return QStringLiteral("transferring");
    case TransferModel::FileDone:         return QStringLiteral("done");
    case TransferModel::FileFailed:       return QStringLiteral("failed");
    case TransferModel::FileSkipped:      return QStringLiteral("skipped");
    default:                              return QStringLiteral("waiting");
    }
}

} // namespace

FileEntry::FileEntry()
    : size(0)
    , transferred(0)
    , status(TransferModel::FileWaiting)
{
}

TransferModel::TransferModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_state(Idle)
    , m_direction(NoDirection)
    , m_totalBytes(0)
    , m_transferredBytes(0)
    , m_dirtyFirst(-1)
    , m_dirtyLast(-1)
    , m_flushTimer(new QTimer(this))
    , m_speedSampleMs(0)
    , m_speedSampleBytes(0)
    , m_bytesPerSecond(0)
{
    m_flushTimer->setInterval(FlushIntervalMs);
    m_flushTimer->setSingleShot(true);
    connect(m_flushTimer, &QTimer::timeout, this, &TransferModel::flush);
}

int TransferModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_files.count();
}

QVariant TransferModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_files.count())
        return QVariant();

    const FileEntry &file = m_files.at(index.row());
    switch (role) {
    case FileIdRole:        return file.id;
    case FileNameRole:      return file.fileName;
    case FileSizeRole:      return file.size;
    case FileTypeRole:      return file.fileType;
    case TransferredRole:   return file.transferred;
    case FileProgressRole:
        return file.size > 0 ? qreal(file.transferred) / qreal(file.size)
                             : (file.status == FileDone ? 1.0 : 0.0);
    case FileStatusRole:    return fileStatusName(file.status);
    case FileErrorRole:     return file.error;
    case LocalPathRole:     return file.localPath;
    default:                return QVariant();
    }
}

QHash<int, QByteArray> TransferModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[FileIdRole] = "fileId";
    roles[FileNameRole] = "fileName";
    roles[FileSizeRole] = "fileSize";
    roles[FileTypeRole] = "fileType";
    roles[TransferredRole] = "transferred";
    roles[FileProgressRole] = "fileProgress";
    roles[FileStatusRole] = "fileStatus";
    roles[FileErrorRole] = "fileError";
    roles[LocalPathRole] = "localPath";
    return roles;
}

TransferModel::State TransferModel::state() const { return m_state; }
TransferModel::Direction TransferModel::direction() const { return m_direction; }
bool TransferModel::active() const
{
    return m_state == Active || m_state == Pending || m_state == Requesting;
}
bool TransferModel::sending() const { return m_direction == Sending; }

QString TransferModel::stateName() const
{
    switch (m_state) {
    case Pending:    return QStringLiteral("pending");
    case Requesting: return QStringLiteral("requesting");
    case Active:    return QStringLiteral("active");
    case Finished:  return QStringLiteral("finished");
    case Failed:    return QStringLiteral("failed");
    case Cancelled: return QStringLiteral("cancelled");
    default:        return QStringLiteral("idle");
    }
}

QString TransferModel::directionName() const
{
    switch (m_direction) {
    case Sending:   return QStringLiteral("send");
    case Receiving: return QStringLiteral("receive");
    default:        return QString();
    }
}

QString TransferModel::peerAlias() const { return m_peerAlias; }
QString TransferModel::peerAddress() const { return m_peerAddress; }
QString TransferModel::peerDeviceType() const { return m_peerDeviceType; }
QString TransferModel::peerFingerprint() const { return m_peerFingerprint; }
QString TransferModel::destination() const { return m_destination; }
QString TransferModel::errorText() const { return m_errorText; }

int TransferModel::fileCount() const { return m_files.count(); }
qint64 TransferModel::totalBytes() const { return m_totalBytes; }
qint64 TransferModel::transferredBytes() const { return m_transferredBytes; }
qint64 TransferModel::bytesPerSecond() const { return m_bytesPerSecond; }

int TransferModel::completedCount() const
{
    int count = 0;
    for (int i = 0; i < m_files.count(); ++i) {
        if (m_files.at(i).status == FileDone)
            ++count;
    }
    return count;
}

int TransferModel::failedCount() const
{
    int count = 0;
    for (int i = 0; i < m_files.count(); ++i) {
        const int status = m_files.at(i).status;
        if (status == FileFailed || status == FileSkipped)
            ++count;
    }
    return count;
}

int TransferModel::currentIndex() const
{
    for (int i = 0; i < m_files.count(); ++i) {
        if (m_files.at(i).status == FileTransferring)
            return i;
    }
    for (int i = 0; i < m_files.count(); ++i) {
        if (m_files.at(i).status == FileWaiting)
            return i;
    }
    return m_files.isEmpty() ? -1 : m_files.count() - 1;
}

qreal TransferModel::progress() const
{
    if (m_totalBytes <= 0) {
        // Zero-byte files still make a transfer that can complete.
        return m_files.isEmpty() ? 0.0
                                 : qreal(completedCount()) / qreal(m_files.count());
    }
    const qreal value = qreal(m_transferredBytes) / qreal(m_totalBytes);
    return value > 1.0 ? 1.0 : value;
}

int TransferModel::secondsRemaining() const
{
    if (m_bytesPerSecond <= 0 || m_totalBytes <= 0 || m_state != Active)
        return -1;
    const qint64 left = m_totalBytes - m_transferredBytes;
    if (left <= 0)
        return 0;
    return int(left / m_bytesPerSecond);
}

int TransferModel::elapsedSeconds() const
{
    return m_clock.isValid() ? int(m_clock.elapsed() / 1000) : 0;
}

void TransferModel::begin(Direction direction, const QString &alias,
                          const QString &address, const QString &deviceType,
                          const QString &fingerprint,
                          const QList<FileEntry> &files)
{
    beginResetModel();
    m_files = files;
    m_direction = direction;
    m_peerAlias = alias;
    m_peerAddress = address;
    m_peerDeviceType = deviceType;
    m_peerFingerprint = fingerprint;
    m_errorText.clear();
    m_destination.clear();
    m_transferredBytes = 0;
    m_bytesPerSecond = 0;
    m_speedSampleBytes = 0;
    m_dirtyFirst = -1;
    m_dirtyLast = -1;
    recomputeTotals();
    endResetModel();

    m_clock.restart();
    m_speedSampleMs = 0;

    emit peerChanged();
    emit progressChanged();
    emit destinationChanged();
}

void TransferModel::setState(State state, const QString &error)
{
    if (m_state == state && m_errorText == error)
        return;

    m_state = state;
    m_errorText = error;

    if (state == Active && !m_clock.isValid())
        m_clock.restart();

    if (state != Active) {
        // Whatever was still folded into the entries has to land before the
        // page switches to its finished layout.
        m_flushTimer->stop();
        flush();
        m_bytesPerSecond = 0;
    }

    emit stateChanged();
    emit progressChanged();
}

void TransferModel::setDestination(const QString &path)
{
    if (m_destination == path)
        return;
    m_destination = path;
    emit destinationChanged();
}

void TransferModel::recomputeTotals()
{
    m_totalBytes = 0;
    for (int i = 0; i < m_files.count(); ++i)
        m_totalBytes += m_files.at(i).size;
}

void TransferModel::touch(int row)
{
    if (m_dirtyFirst < 0 || row < m_dirtyFirst)
        m_dirtyFirst = row;
    if (row > m_dirtyLast)
        m_dirtyLast = row;

    if (!m_flushTimer->isActive())
        m_flushTimer->start();
}

void TransferModel::flush()
{
    if (m_dirtyFirst >= 0) {
        // One span rather than one signal per row. Files move in order, so
        // the span is tight in practice, and a delegate that repaints without
        // having changed costs nothing next to a signal per socket read.
        emit dataChanged(index(m_dirtyFirst, 0), index(m_dirtyLast, 0));
        m_dirtyFirst = -1;
        m_dirtyLast = -1;
    }

    // Speed over a sliding window, not since the start: a transfer that
    // stalls should show it, and an average never would.
    if (m_clock.isValid()) {
        const qint64 now = m_clock.elapsed();
        const qint64 span = now - m_speedSampleMs;
        if (span >= SpeedWindowMs) {
            const qint64 moved = m_transferredBytes - m_speedSampleBytes;
            const qint64 sampled = (moved * 1000) / span;
            // Exponential smoothing, weighted towards the new sample so the
            // readout still reacts within a second or two.
            m_bytesPerSecond = m_bytesPerSecond > 0
                    ? (m_bytesPerSecond + sampled * 2) / 3
                    : sampled;
            m_speedSampleMs = now;
            m_speedSampleBytes = m_transferredBytes;
        }
    }

    emit progressChanged();
}

void TransferModel::setFileToken(int row, const QString &token)
{
    if (row < 0 || row >= m_files.count())
        return;
    m_files[row].token = token;
}

void TransferModel::setFileLocalPath(int row, const QString &path)
{
    if (row < 0 || row >= m_files.count())
        return;
    m_files[row].localPath = path;
    touch(row);
}

void TransferModel::setFileStatus(int row, FileStatus status, const QString &error)
{
    if (row < 0 || row >= m_files.count())
        return;

    FileEntry &file = m_files[row];
    file.status = status;
    file.error = error;

    // A finished file counts for its whole size even when the last progress
    // report never arrived, so the total does not end a few bytes short.
    if (status == FileDone && file.transferred != file.size) {
        m_transferredBytes += file.size - file.transferred;
        file.transferred = file.size;
    }

    touch(row);
    // Status changes are rare and drive the layout, so they go out at once.
    m_flushTimer->stop();
    flush();
}

void TransferModel::setFileTransferred(int row, qint64 bytes)
{
    if (row < 0 || row >= m_files.count())
        return;

    FileEntry &file = m_files[row];
    if (bytes < 0)
        bytes = 0;
    if (bytes > file.size && file.size > 0)
        bytes = file.size;
    if (bytes == file.transferred)
        return;

    m_transferredBytes += bytes - file.transferred;
    file.transferred = bytes;
    touch(row);
}

int TransferModel::indexOfFile(const QString &fileId) const
{
    for (int i = 0; i < m_files.count(); ++i) {
        if (m_files.at(i).id == fileId)
            return i;
    }
    return -1;
}

const FileEntry &TransferModel::entry(int row) const
{
    return m_files.at(row);
}

QList<FileEntry> TransferModel::entries() const
{
    return m_files;
}

QStringList TransferModel::transferredPaths() const
{
    QStringList paths;
    for (int i = 0; i < m_files.count(); ++i) {
        const FileEntry &file = m_files.at(i);
        if (file.status == FileDone && !file.localPath.isEmpty())
            paths.append(file.localPath);
    }
    return paths;
}
