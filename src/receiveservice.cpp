#include "receiveservice.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include "appsettings.h"
#include "certificate.h"
#include "crypto.h"
#include "discovery.h"
#include "knowndevices.h"
#include "historymodel.h"
#include "httpserver.h"
#include "protocol.h"
#include "transfermodel.h"

namespace {

// A file is written under this suffix and renamed on success, so an aborted
// transfer never leaves something that looks like a complete file.
const char *PartSuffix = ".part";

// A session with no traffic at all for this long is over, whatever the sender
// thinks. Restarted on every block of every file.
const int SessionIdleMs = 60 * 1000;

// Generous next to any real transfer, and far below what it would take to
// exhaust memory through the token table.
const int MaxFilesPerSession = 2000;

} // namespace

ReceiveService::Upload::Upload()
    : row(-1)
    , file(0)
{
}

ReceiveService::ReceiveService(AppSettings *settings, Discovery *discovery,
                               TransferModel *transfer, HistoryModel *history,
                               QNetworkAccessManager *network, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_discovery(discovery)
    , m_transfer(transfer)
    , m_history(history)
    , m_network(network)
    , m_server(new HttpServer(this))
    , m_known(0)
    , m_acceptTimer(new QTimer(this))
    , m_idleTimer(new QTimer(this))
{
    connect(m_server, &HttpServer::connectionReady,
            this, &ReceiveService::onConnectionReady);

    m_acceptTimer->setSingleShot(true);
    m_acceptTimer->setInterval(Protocol::AcceptTimeoutMs);
    connect(m_acceptTimer, &QTimer::timeout, this, &ReceiveService::onAcceptTimeout);

    m_idleTimer->setSingleShot(true);
    m_idleTimer->setInterval(SessionIdleMs);
    connect(m_idleTimer, &QTimer::timeout, this, &ReceiveService::onSessionIdle);

    connect(m_settings, &AppSettings::portChanged,
            this, &ReceiveService::onSettingsChanged);
}

void ReceiveService::setKnownDevices(KnownDevices *known)
{
    m_known = known;
}

bool ReceiveService::isListening() const { return m_server->isListening(); }
QString ReceiveService::listenError() const { return m_listenError; }
int ReceiveService::port() const { return m_server->isListening() ? m_server->boundPort()
                                                                  : m_settings->port(); }

QString ReceiveService::endpoint(const char *path) const
{
    return QLatin1String(Protocol::ApiPrefix) + QLatin1String(path);
}

bool ReceiveService::startListening()
{
    if (m_server->isListening())
        return true;

    // The transport has to be settled before the socket is bound, and it has
    // to agree with what the announcement says: a peer told "https" that then
    // gets a plaintext socket simply fails.
    if (m_settings->isEncrypted()) {
        m_server->setIdentity(m_settings->identity().certificate(),
                              m_settings->identity().privateKey());
    } else {
        m_server->setIdentity(QSslCertificate(), QSslKey());
    }

    if (!m_server->start(quint16(m_settings->port()))) {
        m_listenError = m_server->lastError();
        emit listeningChanged();
        return false;
    }

    m_listenError.clear();
    // One line at startup saying what a peer will actually meet: an
    // announcement and a socket that disagree about the transport fail with
    // no symptom on either side. Without the fingerprint, which belongs on
    // the About page where it can be compared, not in the system journal on
    // every launch.
    qWarning("localsend: listening on port %d over %s",
             m_server->boundPort(),
             m_server->isSecure() ? "https" : "http");
    emit listeningChanged();
    return true;
}

void ReceiveService::stopListening()
{
    if (!m_server->isListening())
        return;
    finishSession(QStringLiteral("cancelled"));
    m_server->stop();
    emit listeningChanged();
}

void ReceiveService::onSettingsChanged()
{
    // Rebind on the new port. Anything in flight belonged to the old one.
    if (!m_server->isListening())
        return;
    stopListening();
    startListening();
}

// --- routing -------------------------------------------------------------

void ReceiveService::onConnectionReady(HttpConnection *connection)
{
    connect(connection, &HttpConnection::headersReady,
            this, &ReceiveService::onHeadersReady);
    connect(connection, &HttpConnection::requestReady,
            this, &ReceiveService::onRequestReady);
}

void ReceiveService::onHeadersReady(HttpConnection *connection)
{
    // Only the upload endpoint gets a say before its body arrives, because it
    // is the only one whose body must never reach memory.
    if (connection->path() == endpoint(Protocol::PathUpload))
        handleUploadHeaders(connection);
}

void ReceiveService::onRequestReady(HttpConnection *connection)
{
    const QString path = connection->path();

    if (path == endpoint(Protocol::PathInfo)
            || path == QLatin1String(Protocol::PathLegacyInfo)) {
        handleInfo(connection);
    } else if (path == endpoint(Protocol::PathRegister)) {
        handleRegister(connection);
    } else if (path == endpoint(Protocol::PathPrepareUpload)) {
        handlePrepareUpload(connection);
    } else if (path == endpoint(Protocol::PathUpload)) {
        handleUploadFinished(connection);
    } else if (path == endpoint(Protocol::PathCancel)) {
        handleCancel(connection);
    } else {
        connection->respond(404);
    }
}

void ReceiveService::handleInfo(HttpConnection *connection)
{
    connection->respondJson(200, m_settings->self().toInfoResponse());
}

void ReceiveService::handleRegister(HttpConnection *connection)
{
    const DeviceInfo self =
            m_discovery->registerPeer(connection->jsonBody(), connection->peerAddress());
    connection->respondJson(200, self.toInfoResponse());
}

// --- prepare-upload ------------------------------------------------------

void ReceiveService::handlePrepareUpload(HttpConnection *connection)
{
    if (!m_settings->receiveEnabled()) {
        connection->respond(403);
        return;
    }

    // One session at a time, which is what 409 is for. Without this a second
    // sender would overwrite the first one's tokens mid-transfer.
    if (m_pending || !m_sessionId.isEmpty()) {
        connection->respond(409);
        return;
    }

    // The PIN is short enough to be searched exhaustively over the network in
    // minutes, so the hash's work factor is not the defence - this is. An
    // address that keeps guessing is answered with 429 and told when to come
    // back, long before it has covered a meaningful part of the space.
    const QString peerAddress = connection->peerAddress();
    if (!m_pinAttempts.allow(peerAddress)) {
        connection->addHeader(
            "Retry-After", QByteArray::number(m_pinAttempts.retryAfter(peerAddress)));
        connection->respond(429);
        return;
    }

    if (!m_settings->checkPin(connection->query(QStringLiteral("pin")))) {
        m_pinAttempts.recordFailure(peerAddress);
        connection->respond(401);
        return;
    }
    m_pinAttempts.recordSuccess(peerAddress);

    const QJsonObject body = connection->jsonBody();
    const QJsonObject info = body.value(QStringLiteral("info")).toObject();
    const QJsonObject files = body.value(QStringLiteral("files")).toObject();

    if (info.isEmpty()) {
        connection->respond(400);
        return;
    }
    if (files.isEmpty()) {
        // Nothing to transfer: the spec's "finished, no action needed".
        connection->respond(204);
        return;
    }
    if (files.count() > MaxFilesPerSession) {
        // Every declared file becomes a model row and a token held for the
        // session. A peer is free to claim a million of them; we are not
        // obliged to allocate for it.
        connection->respond(400);
        return;
    }

    DeviceInfo peer = DeviceInfo::fromPayload(info);
    peer.address = connection->peerAddress();

    // A sender's claimed identity is worth something only if the key it
    // handshook with is the one it claims. Without this the fingerprint in
    // the body is a string anybody can put anything in, and the accept prompt
    // would name whoever the sender felt like being.
    //
    // Only checked when a certificate was actually presented: a plain-HTTP
    // peer has none, and there is nothing to verify rather than something to
    // reject.
    const QSslCertificate presented = connection->peerCertificate();
    if (!presented.isNull()) {
        const QString observed = Certificate::fingerprintOf(presented);
        if (!Crypto::equalsFold(observed, peer.fingerprint)) {
            qWarning("localsend: refusing %s, it claims fingerprint %s but "
                     "handshook with %s",
                     qPrintable(peer.address), qPrintable(peer.fingerprint),
                     qPrintable(observed));
            connection->respond(403);
            return;
        }
    }
    // A sender we have never seen still belongs in the device list, and this
    // is often how a device on a multicast-hostile network first appears.
    m_discovery->registerPeer(info, connection->peerAddress());

    QList<FileEntry> entries;
    const QStringList ids = files.keys();
    for (int i = 0; i < ids.count(); ++i) {
        const QJsonObject descriptor = files.value(ids.at(i)).toObject();

        FileEntry entry;
        entry.id = descriptor.value(QStringLiteral("id")).toString(ids.at(i));
        entry.fileName = descriptor.value(QStringLiteral("fileName")).toString();
        entry.size = qint64(descriptor.value(QStringLiteral("size")).toDouble());
        entry.fileType = descriptor.value(QStringLiteral("fileType")).toString();
        entry.sha256 = descriptor.value(QStringLiteral("sha256")).toString();
        entry.preview = descriptor.value(QStringLiteral("preview")).toString();

        if (entry.fileName.isEmpty())
            entry.fileName = QStringLiteral("file");
        if (entry.size < 0)
            entry.size = 0;

        entries.append(entry);
    }

    m_peer = peer;
    m_transfer->begin(TransferModel::Receiving, peer.alias, peer.address,
                      peer.deviceType, entries);
    m_transfer->setState(TransferModel::Pending);

    m_pending = connection;
    connect(connection, &HttpConnection::closed,
            this, &ReceiveService::onPendingClosed);

    if (m_settings->quickSave()) {
        accept();
        return;
    }

    m_acceptTimer->start();
    emit requestArrived(peer.alias, entries.count(), m_transfer->totalBytes());
}

void ReceiveService::accept()
{
    if (!m_pending)
        return;

    const QString directory = prepareDirectory();
    if (directory.isEmpty()) {
        m_pending->respond(500);
        releasePending();
        m_acceptTimer->stop();
        m_transfer->setState(TransferModel::Failed,
                             tr("Cannot write to the destination folder"));
        return;
    }

    m_sessionId = Protocol::generateToken();
    m_sessionAddress = m_pending->peerAddress();
    m_sessionDirectory = directory;
    m_tokens.clear();

    QJsonObject tokens;
    for (int row = 0; row < m_transfer->fileCount(); ++row) {
        const QString token = Protocol::generateToken();
        const QString fileId = m_transfer->entry(row).id;
        m_transfer->setFileToken(row, token);
        m_tokens.insert(fileId, token);
        tokens.insert(fileId, token);
    }

    QJsonObject response;
    response.insert(QStringLiteral("sessionId"), m_sessionId);
    response.insert(QStringLiteral("files"), tokens);
    m_pending->respondJson(200, response);

    // From here the parked connection is just a socket closing: the session
    // is what matters, and it must not be torn down when that happens.
    releasePending();
    m_acceptTimer->stop();

    m_transfer->setDestination(m_sessionDirectory);
    m_transfer->setState(TransferModel::Active);
    m_idleTimer->start();
    emit transferStarted();
}

void ReceiveService::decline()
{
    if (!m_pending)
        return;

    m_pending->respond(403);
    releasePending();
    m_acceptTimer->stop();
    m_transfer->setState(TransferModel::Idle);
}

void ReceiveService::onAcceptTimeout()
{
    if (!m_pending)
        return;

    // The sender has been holding a socket open all this while; letting it go
    // is kinder than leaving it to its own timeout.
    m_pending->respond(403);
    releasePending();
    m_transfer->setState(TransferModel::Idle);
}

void ReceiveService::onPendingClosed(HttpConnection *connection)
{
    if (m_pending != connection)
        return;

    // The sender gave up while the request was on screen.
    m_pending.clear();
    m_acceptTimer->stop();
    if (m_sessionId.isEmpty())
        m_transfer->setState(TransferModel::Idle);
}

void ReceiveService::releasePending()
{
    if (m_pending) {
        disconnect(m_pending.data(), &HttpConnection::closed,
                   this, &ReceiveService::onPendingClosed);
    }
    m_pending.clear();
}

// --- upload --------------------------------------------------------------

void ReceiveService::handleUploadHeaders(HttpConnection *connection)
{
    const QString sessionId = connection->query(QStringLiteral("sessionId"));
    const QString fileId = connection->query(QStringLiteral("fileId"));
    const QString token = connection->query(QStringLiteral("token"));

    if (sessionId.isEmpty() || fileId.isEmpty() || token.isEmpty()) {
        connection->respond(400);
        return;
    }
    if (m_sessionId.isEmpty() || sessionId != m_sessionId) {
        connection->respond(409);
        return;
    }
    // The token is the capability; the address check stops another host on the
    // network from spending it if it ever leaks.
    if (!Crypto::equals(m_tokens.value(fileId), token)
            || connection->peerAddress() != m_sessionAddress) {
        connection->respond(403);
        return;
    }

    const int row = m_transfer->indexOfFile(fileId);
    if (row < 0) {
        connection->respond(400);
        return;
    }
    // A file the sender uploads twice would otherwise reopen and truncate the
    // one already on disk.
    if (m_transfer->entry(row).status == TransferModel::FileDone) {
        connection->respond(200);
        return;
    }

    const QString finalPath = reserveFilePath(m_sessionDirectory,
                                              m_transfer->entry(row).fileName);
    Upload upload;
    upload.row = row;
    upload.finalPath = finalPath;
    upload.partPath = finalPath + QLatin1String(PartSuffix);
    upload.file = new QFile(upload.partPath, this);

    if (!upload.file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        delete upload.file;
        m_transfer->setFileStatus(row, TransferModel::FileFailed,
                                  tr("Cannot write the file"));
        connection->respond(500);
        return;
    }

    m_uploads.insert(connection, upload);

    // Wired before the body is accepted, not after. streamBodyTo() rejects a
    // declared length that is already over the ceiling, and that answer can
    // close the socket inside the same call - which fires closed() while this
    // handler is still running. Connecting afterwards would leave the entry
    // in m_uploads pointing at a connection that has since deleted itself,
    // and the next pass over that table would touch freed memory.
    connect(connection, &HttpConnection::closed,
            this, &ReceiveService::onUploadClosed);
    connect(connection, &HttpConnection::bodyProgress,
            this, [this, connection](qint64 received, qint64) {
        const QHash<HttpConnection *, Upload>::const_iterator it =
                m_uploads.constFind(connection);
        if (it == m_uploads.constEnd())
            return;
        m_transfer->setFileTransferred(it.value().row, received);
        m_idleTimer->start();
    });

    m_transfer->setFileStatus(row, TransferModel::FileTransferring);
    m_idleTimer->start();

    // Bounded by what the sender said this file was when it asked permission.
    // The size in prepare-upload is what the person saw and agreed to; the
    // bytes arrive in a different request, and only this makes the two match.
    connection->streamBodyTo(upload.file, m_transfer->entry(row).size);
}

void ReceiveService::handleUploadFinished(HttpConnection *connection)
{
    const QHash<HttpConnection *, Upload>::iterator it = m_uploads.find(connection);
    if (it == m_uploads.end()) {
        // Rejected at header time, and already answered.
        if (!connection->hasResponded())
            connection->respond(400);
        return;
    }

    Upload upload = it.value();
    m_uploads.erase(it);
    disconnect(connection, &HttpConnection::closed,
               this, &ReceiveService::onUploadClosed);

    upload.file->flush();
    upload.file->close();
    const bool complete = upload.file->size() >= m_transfer->entry(upload.row).size;
    delete upload.file;

    if (!complete) {
        QFile::remove(upload.partPath);
        m_transfer->setFileStatus(upload.row, TransferModel::FileFailed,
                                  tr("Transfer was cut short"));
        connection->respond(500);
    } else {
        QFile::remove(upload.finalPath);
        // Never executable, whatever it is called. Nothing here would set the
        // bit, but a received file is the one thing on this device chosen
        // entirely by somebody else, so it is worth being explicit rather
        // than relying on the umask staying what it is today.
        QFile::setPermissions(upload.partPath,
                              QFile::ReadOwner | QFile::WriteOwner
                              | QFile::ReadGroup | QFile::ReadOther);
        if (QFile::rename(upload.partPath, upload.finalPath)) {
            m_transfer->setFileLocalPath(upload.row, upload.finalPath);
            m_transfer->setFileStatus(upload.row, TransferModel::FileDone);
        } else {
            // The bytes are safe under the .part name, so keep them rather
            // than throw the transfer away over a rename.
            m_transfer->setFileLocalPath(upload.row, upload.partPath);
            m_transfer->setFileStatus(upload.row, TransferModel::FileDone);
        }
        connection->respond(200);
    }

    m_idleTimer->start();

    if (m_transfer->completedCount() + m_transfer->failedCount()
            >= m_transfer->fileCount()) {
        finishSession(m_transfer->failedCount() > 0 ? QStringLiteral("failed")
                                                    : QStringLiteral("finished"));
    }
}

void ReceiveService::onUploadClosed(HttpConnection *connection)
{
    // Only reached when the socket died before the body was complete: the
    // normal path disconnects this first.
    const QHash<HttpConnection *, Upload>::iterator it = m_uploads.find(connection);
    if (it == m_uploads.end())
        return;

    const Upload upload = it.value();
    m_uploads.erase(it);

    upload.file->close();
    delete upload.file;
    QFile::remove(upload.partPath);

    m_transfer->setFileStatus(upload.row, TransferModel::FileFailed,
                              tr("The sender disconnected"));

    if (m_transfer->completedCount() + m_transfer->failedCount()
            >= m_transfer->fileCount()) {
        finishSession(QStringLiteral("failed"));
    }
}

void ReceiveService::closeUpload(HttpConnection *connection, bool keepFile)
{
    const QHash<HttpConnection *, Upload>::iterator it = m_uploads.find(connection);
    if (it == m_uploads.end())
        return;

    const Upload upload = it.value();
    m_uploads.erase(it);

    disconnect(connection, &HttpConnection::closed,
               this, &ReceiveService::onUploadClosed);

    upload.file->close();
    delete upload.file;
    if (!keepFile)
        QFile::remove(upload.partPath);
}

// --- cancel and teardown -------------------------------------------------

void ReceiveService::handleCancel(HttpConnection *connection)
{
    const QString sessionId = connection->query(QStringLiteral("sessionId"));
    connection->respond(200);

    if (m_pending && m_sessionId.isEmpty()) {
        releasePending();
        m_acceptTimer->stop();
        m_transfer->setState(TransferModel::Cancelled);
        return;
    }

    if (!m_sessionId.isEmpty() && (sessionId.isEmpty() || sessionId == m_sessionId))
        finishSession(QStringLiteral("cancelled"));
}

void ReceiveService::cancel()
{
    if (m_pending && m_sessionId.isEmpty()) {
        decline();
        return;
    }
    if (m_sessionId.isEmpty())
        return;

    notifyPeerCancelled();
    finishSession(QStringLiteral("cancelled"));
}

void ReceiveService::notifyPeerCancelled()
{
    if (m_peer.address.isEmpty() || m_sessionId.isEmpty())
        return;

    QUrl url(m_peer.apiBase() + QLatin1String(Protocol::PathCancel));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("sessionId"), m_sessionId);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    QNetworkReply *reply = m_network->post(request, QByteArray());
    connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
}

void ReceiveService::finishSession(const QString &status)
{
    const bool hadSession = !m_sessionId.isEmpty();
    if (!hadSession && !m_pending)
        return;

    m_idleTimer->stop();
    m_acceptTimer->stop();

    // Anything still streaming is cut off; its bytes are incomplete by
    // definition, so the part file goes with it.
    const QList<HttpConnection *> open = m_uploads.keys();
    for (int i = 0; i < open.count(); ++i) {
        HttpConnection *connection = open.at(i);
        const int row = m_uploads.value(connection).row;
        closeUpload(connection, false);
        m_transfer->setFileStatus(row, TransferModel::FileFailed,
                                  tr("Transfer stopped"));
        connection->respond(500);
    }

    if (m_pending) {
        m_pending->respond(403);
        releasePending();
    }

    if (!hadSession) {
        m_transfer->setState(TransferModel::Cancelled);
        return;
    }

    // Files the sender never got round to.
    for (int row = 0; row < m_transfer->fileCount(); ++row) {
        const int fileStatus = m_transfer->entry(row).status;
        if (fileStatus == TransferModel::FileWaiting
                || fileStatus == TransferModel::FileTransferring) {
            m_transfer->setFileStatus(row, TransferModel::FileSkipped);
        }
    }

    const int done = m_transfer->completedCount();
    const int total = m_transfer->fileCount();

    if (status == QLatin1String("cancelled"))
        m_transfer->setState(TransferModel::Cancelled);
    else if (done == total)
        m_transfer->setState(TransferModel::Finished);
    else if (done > 0)
        m_transfer->setState(TransferModel::Failed,
                             tr("%1 of %2 files were received").arg(done).arg(total));
    else
        m_transfer->setState(TransferModel::Failed, tr("Nothing was received"));

    // Recorded only once something actually arrived from this key, not on
    // discovery: remembering everything that announces itself would let an
    // impostor claim a name simply by getting there first.
    if (m_known && done > 0)
        m_known->remember(m_peer.fingerprint, m_peer.alias);

    if (m_settings->historyEnabled() && done > 0) {
        HistoryModel::Record record;
        record.id = m_sessionId;
        record.direction = QStringLiteral("receive");
        record.peerAlias = m_peer.alias;
        record.peerDeviceType = m_peer.deviceType;
        record.timestamp = QDateTime::currentDateTime();
        record.destination = m_sessionDirectory;
        record.status = m_transfer->stateName();
        for (int row = 0; row < total; ++row) {
            if (m_transfer->entry(row).status == TransferModel::FileDone) {
                record.fileNames.append(m_transfer->entry(row).fileName);
                record.totalBytes += m_transfer->entry(row).size;
            }
        }
        m_history->append(record);
    }

    const QString destination = m_sessionDirectory;
    const int fileCount = done;

    m_sessionId.clear();
    m_sessionAddress.clear();
    m_sessionDirectory.clear();
    m_tokens.clear();

    emit transferFinished(m_transfer->stateName(), fileCount, destination);
}

void ReceiveService::onSessionIdle()
{
    if (m_sessionId.isEmpty())
        return;
    finishSession(QStringLiteral("failed"));
}

// --- destination handling ------------------------------------------------

QString ReceiveService::prepareDirectory()
{
    QString path = m_settings->destination();

    if (m_settings->folderPerSender() && !m_peer.alias.isEmpty())
        path += QLatin1Char('/') + sanitizeFileName(m_peer.alias);

    QDir directory;
    if (!directory.mkpath(path))
        return QString();

    // mkpath succeeds on a read-only tree that already exists, so the only
    // honest test is whether we can create something in it.
    QFile probe(path + QStringLiteral("/.localsend-write-test"));
    if (!probe.open(QIODevice::WriteOnly))
        return QString();
    probe.close();
    probe.remove();

    return path;
}

QString ReceiveService::sanitizeFileName(const QString &name)
{
    // A peer chooses this string. "../../.ssh/authorized_keys" is a perfectly
    // well-formed value for it, so the directory part goes first and the
    // result is checked for the traversal names that survive that.
    // Controls, bidi overrides and angle brackets go first. A name carrying
    // U+202E draws on screen as one extension while being another on disk,
    // which is how an accept prompt is made to say "holiday.jpg" for a shell
    // script; the brackets are about the same name being rendered as markup.
    QString clean = QFileInfo(Protocol::sanitizeText(name, 200)).fileName();
    clean.replace(QLatin1Char('\\'), QLatin1Char('_'));
    clean.replace(QLatin1Char('/'), QLatin1Char('_'));

    if (clean.isEmpty() || clean == QLatin1String(".") || clean == QLatin1String(".."))
        clean = QStringLiteral("received-file");

    // Most filesystems stop at 255 bytes and the suffix matters more than the
    // tail of a long name.
    if (clean.length() > 200) {
        const QFileInfo info(clean);
        const QString suffix = info.suffix();
        clean = clean.left(suffix.isEmpty() ? 200 : 200 - suffix.length() - 1);
        if (!suffix.isEmpty())
            clean += QLatin1Char('.') + suffix;
    }

    return clean;
}

QString ReceiveService::reserveFilePath(const QString &directory,
                                        const QString &fileName) const
{
    const QString clean = sanitizeFileName(fileName);
    QString candidate = directory + QLatin1Char('/') + clean;

    if (!QFile::exists(candidate)
            && !QFile::exists(candidate + QLatin1String(PartSuffix))) {
        return candidate;
    }

    const QFileInfo info(clean);
    QString base = info.completeBaseName();
    QString suffix = info.suffix();
    if (base.isEmpty()) {
        // A name that is nothing but an extension, ".bashrc" being the usual
        // one: treat the whole thing as the stem.
        base = clean;
        suffix.clear();
    }

    for (int n = 2; n < 1000; ++n) {
        QString name = base + QStringLiteral(" (") + QString::number(n)
                     + QLatin1Char(')');
        if (!suffix.isEmpty())
            name += QLatin1Char('.') + suffix;

        candidate = directory + QLatin1Char('/') + name;
        if (!QFile::exists(candidate)
                && !QFile::exists(candidate + QLatin1String(PartSuffix))) {
            return candidate;
        }
    }

    return candidate;
}
