#include "sendservice.h"

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

#include "appsettings.h"
#include "historymodel.h"
#include "protocol.h"
#include "tlsclient.h"
#include "transfermodel.h"

namespace {

QString localPathOf(const QString &pathOrUrl)
{
    if (pathOrUrl.startsWith(QLatin1String("file://")))
        return QUrl(pathOrUrl).toLocalFile();
    return pathOrUrl;
}

} // namespace

SendService::SendService(AppSettings *settings, TransferModel *transfer,
                         HistoryModel *history, QNetworkAccessManager *network,
                         QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_transfer(transfer)
    , m_history(history)
    , m_network(network)
    , m_currentFile(0)
    , m_currentRow(-1)
    , m_busy(false)
    , m_cancelled(false)
    , m_pinAttempted(false)
{
}

bool SendService::isBusy() const
{
    return m_busy;
}

void SendService::sendFiles(const QVariantMap &device, const QStringList &paths)
{
    if (m_busy)
        return;

    DeviceInfo peer;
    peer.alias = device.value(QStringLiteral("alias")).toString();
    peer.deviceModel = device.value(QStringLiteral("hardware")).toString();
    peer.deviceType = device.value(QStringLiteral("deviceType")).toString();
    peer.fingerprint = device.value(QStringLiteral("fingerprint")).toString();
    peer.address = device.value(QStringLiteral("address")).toString();
    peer.port = device.value(QStringLiteral("port"), Protocol::DefaultPort).toInt();
    peer.protocol = device.value(QStringLiteral("protocol"),
                                 QStringLiteral("http")).toString();

    if (peer.address.isEmpty() || paths.isEmpty())
        return;

    QMimeDatabase mimeDatabase;
    QList<FileEntry> entries;

    for (int i = 0; i < paths.count(); ++i) {
        const QString path = localPathOf(paths.at(i));
        const QFileInfo info(path);
        if (!info.exists() || !info.isFile() || !info.isReadable())
            continue;

        FileEntry entry;
        // The id only has to be unique within the request; the index keeps it
        // stable across the two round trips even for duplicate file names.
        entry.id = QString::number(i) + QLatin1Char('-') + Protocol::generateToken().left(12);
        entry.fileName = info.fileName();
        entry.size = info.size();
        entry.localPath = path;
        // Extension matching only: sniffing the content would mean reading
        // every file twice, and the receiver treats this as a hint anyway.
        entry.fileType = mimeDatabase.mimeTypeForFile(
            info, QMimeDatabase::MatchExtension).name();
        entries.append(entry);
    }

    if (entries.isEmpty())
        return;

    m_peer = peer;
    m_sessionId.clear();
    m_cancelled = false;
    m_pinAttempted = false;
    m_busy = true;
    emit busyChanged();

    m_transfer->begin(TransferModel::Sending, peer.alias, peer.address,
                      peer.deviceType, entries);
    m_transfer->setState(TransferModel::Requesting);

    requestUpload(QString());
}

void SendService::requestUpload(const QString &pin)
{
    QJsonObject files;
    for (int row = 0; row < m_transfer->fileCount(); ++row) {
        const FileEntry &entry = m_transfer->entry(row);

        QJsonObject descriptor;
        descriptor.insert(QStringLiteral("id"), entry.id);
        descriptor.insert(QStringLiteral("fileName"), entry.fileName);
        descriptor.insert(QStringLiteral("size"), double(entry.size));
        descriptor.insert(QStringLiteral("fileType"), entry.fileType);
        // sha256 and preview are optional, and both would mean reading every
        // file before the transfer even starts.
        files.insert(entry.id, descriptor);
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("info"), m_settings->self().toRegisterBody());
    payload.insert(QStringLiteral("files"), files);

    QUrl url(m_peer.apiBase() + QLatin1String(Protocol::PathPrepareUpload));
    if (!pin.isEmpty()) {
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("pin"), pin);
        url.setQuery(query);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    TlsClient::configure(request);

    m_reply = m_network->post(request,
                              QJsonDocument(payload).toJson(QJsonDocument::Compact));
    // The file list and the PIN go over this request, so it is pinned to the
    // fingerprint the device announced. A peer that cannot present that
    // certificate does not get told what we were about to send it.
    TlsClient::pin(m_reply.data(), m_peer.fingerprint);
    connect(m_reply.data(), &QNetworkReply::finished,
            this, &SendService::onPrepareFinished);
}

void SendService::submitPin(const QString &pin)
{
    if (!m_busy || pin.isEmpty())
        return;
    m_pinAttempted = true;
    m_transfer->setState(TransferModel::Requesting);
    requestUpload(pin);
}

void SendService::onPrepareFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();
    if (m_reply == reply)
        m_reply.clear();

    if (m_cancelled)
        return;

    const int status = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError && status == 0) {
        failTransfer(tr("Could not reach %1").arg(m_peer.alias));
        return;
    }

    switch (status) {
    case 200:
        break;
    case 204:
        // The receiver already has everything; nothing left to do.
        m_transfer->setState(TransferModel::Finished);
        m_busy = false;
        emit busyChanged();
        emit finished(QStringLiteral("finished"), 0);
        return;
    case 401:
        m_transfer->setState(TransferModel::Idle);
        emit pinRequired(m_pinAttempted);
        return;
    case 403:
        m_transfer->setState(TransferModel::Cancelled);
        m_busy = false;
        emit busyChanged();
        emit declined();
        emit finished(QStringLiteral("declined"), 0);
        return;
    case 409:
        failTransfer(tr("%1 is busy with another transfer").arg(m_peer.alias));
        return;
    case 429:
        failTransfer(tr("%1 is refusing new requests").arg(m_peer.alias));
        return;
    default:
        failTransfer(tr("%1 refused the transfer (%2)").arg(m_peer.alias).arg(status));
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    if (!document.isObject()) {
        failTransfer(tr("%1 sent an unreadable reply").arg(m_peer.alias));
        return;
    }

    const QJsonObject response = document.object();
    m_sessionId = response.value(QStringLiteral("sessionId")).toString();
    const QJsonObject tokens = response.value(QStringLiteral("files")).toObject();

    if (m_sessionId.isEmpty() || tokens.isEmpty()) {
        failTransfer(tr("%1 sent an unreadable reply").arg(m_peer.alias));
        return;
    }

    // A receiver is free to accept only some of the files. The ones it left
    // out are skipped rather than treated as an error.
    int accepted = 0;
    for (int row = 0; row < m_transfer->fileCount(); ++row) {
        const QString token = tokens.value(m_transfer->entry(row).id).toString();
        if (token.isEmpty()) {
            m_transfer->setFileStatus(row, TransferModel::FileSkipped);
        } else {
            m_transfer->setFileToken(row, token);
            ++accepted;
        }
    }

    if (accepted == 0) {
        m_transfer->setState(TransferModel::Finished);
        m_busy = false;
        emit busyChanged();
        emit finished(QStringLiteral("finished"), 0);
        return;
    }

    m_transfer->setState(TransferModel::Active);
    m_currentRow = -1;
    startNextFile();
}

void SendService::startNextFile()
{
    closeCurrentFile();

    if (m_cancelled)
        return;

    int row = m_currentRow + 1;
    while (row < m_transfer->fileCount()
           && m_transfer->entry(row).status != TransferModel::FileWaiting) {
        ++row;
    }

    if (row >= m_transfer->fileCount()) {
        completeTransfer();
        return;
    }

    m_currentRow = row;
    const FileEntry entry = m_transfer->entry(row);

    m_currentFile = new QFile(entry.localPath, this);
    if (!m_currentFile->open(QIODevice::ReadOnly)) {
        delete m_currentFile;
        m_currentFile = 0;
        m_transfer->setFileStatus(row, TransferModel::FileFailed,
                                  tr("Could not open the file"));
        startNextFile();
        return;
    }

    QUrl url(m_peer.apiBase() + QLatin1String(Protocol::PathUpload));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("sessionId"), m_sessionId);
    query.addQueryItem(QStringLiteral("fileId"), entry.id);
    query.addQueryItem(QStringLiteral("token"), entry.token);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/octet-stream"));
    request.setHeader(QNetworkRequest::ContentLengthHeader, entry.size);
    TlsClient::configure(request);

    m_transfer->setFileStatus(row, TransferModel::FileTransferring);

    // The device stays ours: QNetworkAccessManager reads from it but never
    // owns it, so it has to outlive the reply and be closed by hand.
    m_reply = m_network->post(request, m_currentFile);
    // This is the request carrying the file itself. Nothing goes out over it
    // until the certificate has been matched against the announcement.
    TlsClient::pin(m_reply.data(), m_peer.fingerprint);
    connect(m_reply.data(), &QNetworkReply::finished,
            this, &SendService::onUploadFinished);

    const int trackedRow = row;
    connect(m_reply.data(), &QNetworkReply::uploadProgress,
            this, [this, trackedRow](qint64 sent, qint64) {
        m_transfer->setFileTransferred(trackedRow, sent);
    });
}

void SendService::onUploadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply)
        return;
    reply->deleteLater();
    if (m_reply == reply)
        m_reply.clear();

    if (m_cancelled)
        return;

    const int status = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() == QNetworkReply::NoError && status >= 200 && status < 300) {
        m_transfer->setFileStatus(m_currentRow, TransferModel::FileDone);
    } else if (status == 409) {
        // The session is gone on the other side; the rest would fail too.
        m_transfer->setFileStatus(m_currentRow, TransferModel::FileFailed,
                                  tr("The transfer was stopped"));
        failTransfer(tr("%1 stopped the transfer").arg(m_peer.alias));
        return;
    } else {
        m_transfer->setFileStatus(m_currentRow, TransferModel::FileFailed,
                                  reply->errorString());
    }

    startNextFile();
}

void SendService::closeCurrentFile()
{
    if (!m_currentFile)
        return;
    m_currentFile->close();

    // deleteLater, not delete: the reply that was reading from this device is
    // itself only queued for deletion, and it still holds a pointer to it.
    // Both are queued, deletions run in order, and the reply goes first
    // because its deleteLater() was called first.
    m_currentFile->deleteLater();
    m_currentFile = 0;
}

void SendService::completeTransfer()
{
    const int done = m_transfer->completedCount();
    const int total = m_transfer->fileCount();

    if (done == total)
        m_transfer->setState(TransferModel::Finished);
    else if (done > 0)
        m_transfer->setState(TransferModel::Failed,
                             tr("%1 of %2 files were sent").arg(done).arg(total));
    else
        m_transfer->setState(TransferModel::Failed, tr("Nothing was sent"));

    writeHistory();

    m_busy = false;
    m_sessionId.clear();
    emit busyChanged();
    emit finished(m_transfer->stateName(), done);
}

void SendService::failTransfer(const QString &message)
{
    closeCurrentFile();

    for (int row = 0; row < m_transfer->fileCount(); ++row) {
        if (m_transfer->entry(row).status == TransferModel::FileWaiting)
            m_transfer->setFileStatus(row, TransferModel::FileSkipped);
    }

    m_transfer->setState(TransferModel::Failed, message);
    if (m_transfer->completedCount() > 0)
        writeHistory();

    m_busy = false;
    m_sessionId.clear();
    emit busyChanged();
    emit finished(QStringLiteral("failed"), m_transfer->completedCount());
}

void SendService::cancel()
{
    if (!m_busy)
        return;

    m_cancelled = true;

    if (m_reply)
        m_reply->abort();
    closeCurrentFile();
    notifyPeerCancelled();

    for (int row = 0; row < m_transfer->fileCount(); ++row) {
        const int status = m_transfer->entry(row).status;
        if (status == TransferModel::FileWaiting
                || status == TransferModel::FileTransferring) {
            m_transfer->setFileStatus(row, TransferModel::FileSkipped);
        }
    }

    m_transfer->setState(TransferModel::Cancelled);
    if (m_transfer->completedCount() > 0)
        writeHistory();

    m_busy = false;
    m_sessionId.clear();
    emit busyChanged();
    emit finished(QStringLiteral("cancelled"), m_transfer->completedCount());
}

void SendService::notifyPeerCancelled()
{
    if (m_sessionId.isEmpty() || m_peer.address.isEmpty())
        return;

    QUrl url(m_peer.apiBase() + QLatin1String(Protocol::PathCancel));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("sessionId"), m_sessionId);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    TlsClient::configure(request);

    QNetworkReply *reply = m_network->post(request, QByteArray());
    TlsClient::pin(reply, m_peer.fingerprint);
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void SendService::writeHistory()
{
    if (!m_settings->historyEnabled())
        return;

    HistoryModel::Record record;
    record.id = m_sessionId.isEmpty() ? Protocol::generateToken() : m_sessionId;
    record.direction = QStringLiteral("send");
    record.peerAlias = m_peer.alias;
    record.peerDeviceType = m_peer.deviceType;
    record.timestamp = QDateTime::currentDateTime();
    record.status = m_transfer->stateName();

    for (int row = 0; row < m_transfer->fileCount(); ++row) {
        const FileEntry &entry = m_transfer->entry(row);
        if (entry.status != TransferModel::FileDone)
            continue;
        record.fileNames.append(entry.fileName);
        record.totalBytes += entry.size;
    }

    if (!record.fileNames.isEmpty())
        m_history->append(record);
}
