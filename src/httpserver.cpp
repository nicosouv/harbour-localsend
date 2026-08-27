#include "httpserver.h"

#include <QIODevice>
#include <QJsonDocument>
#include <QPair>
#include <QSslSocket>
#include <QTcpSocket>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {

const int MaxHeadSize = 32 * 1024;
const qint64 MaxBufferedBody = 16LL * 1024 * 1024;

// Long enough for a slow phone on a busy network, short enough that a peer
// that vanished mid-request does not hold a socket forever. Restarted on
// every byte, and stopped once the request is complete.
const int IdleTimeoutMs = 60 * 1000;

// Caps how much the kernel hands us per readyRead. Uploads are written to
// disk synchronously, so a smaller window here keeps memory flat when the
// network is faster than the storage.
const int SocketReadBuffer = 256 * 1024;

// The protocol runs one session at a time and a session uses one socket at a
// time, so anything past a handful is either a badly behaved client or an
// attempt to exhaust our file descriptors.
const int MaxConnections = 24;

// Long enough for a slow handshake on a busy radio, short enough that a peer
// that opens a socket and says nothing does not hold a slot.
const int HandshakeTimeoutMs = 15 * 1000;

QByteArray reasonPhrase(int status)
{
    switch (status) {
    case 100: return "Continue";
    case 200: return "OK";
    case 204: return "No Content";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 408: return "Request Timeout";
    case 409: return "Conflict";
    case 413: return "Payload Too Large";
    case 429: return "Too Many Requests";
    case 431: return "Request Header Fields Too Large";
    case 500: return "Internal Server Error";
    default:  return "Unknown";
    }
}

} // namespace

// --- HttpConnection ------------------------------------------------------

HttpConnection::HttpConnection(QTcpSocket *socket, QObject *parent)
    : QObject(parent)
    , m_socket(socket)
    , m_idleTimer(new QTimer(this))
    , m_state(ReadingHeaders)
    , m_sink(0)
    , m_contentLength(0)
    , m_bodyReceived(0)
    , m_chunked(false)
    , m_chunkState(ChunkSize)
    , m_chunkRemaining(0)
    , m_bodyComplete(false)
    , m_closedEmitted(false)
{
    m_socket->setParent(this);
    m_socket->setReadBufferSize(SocketReadBuffer);

    m_peerAddress = m_socket->peerAddress().toString();
    // An IPv4 peer reaching a dual-stack listener is spelled ::ffff:a.b.c.d,
    // and the upload handler compares this against the address that opened
    // the session, so both sides have to be in the same notation.
    if (m_peerAddress.startsWith(QLatin1String("::ffff:")))
        m_peerAddress = m_peerAddress.mid(7);

    connect(m_socket, &QTcpSocket::readyRead, this, &HttpConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &HttpConnection::onDisconnected);

    m_idleTimer->setSingleShot(true);
    m_idleTimer->setInterval(IdleTimeoutMs);
    connect(m_idleTimer, &QTimer::timeout, this, &HttpConnection::onIdleTimeout);
    m_idleTimer->start();

    // Over TLS the request usually arrives inside the same event loop pass as
    // the handshake, so by the time this object exists readyRead has already
    // been and gone. Whatever is sitting in the socket has to be picked up
    // explicitly or the connection stalls until it times out.
    if (m_socket->bytesAvailable() > 0)
        QMetaObject::invokeMethod(this, "onReadyRead", Qt::QueuedConnection);
}

HttpConnection::~HttpConnection()
{
}

QString HttpConnection::method() const { return m_method; }
QString HttpConnection::path() const { return m_path; }
QByteArray HttpConnection::body() const { return m_body; }
QString HttpConnection::peerAddress() const { return m_peerAddress; }
qint64 HttpConnection::contentLength() const { return m_contentLength; }
qint64 HttpConnection::bodyReceived() const { return m_bodyReceived; }
bool HttpConnection::hasResponded() const { return m_state == Responded; }

QString HttpConnection::query(const QString &key) const
{
    return m_query.value(key);
}

bool HttpConnection::hasQuery(const QString &key) const
{
    return m_query.contains(key);
}

QByteArray HttpConnection::header(const QByteArray &name) const
{
    return m_headers.value(name.toLower());
}

QJsonObject HttpConnection::jsonBody() const
{
    const QJsonDocument document = QJsonDocument::fromJson(m_body);
    return document.isObject() ? document.object() : QJsonObject();
}

void HttpConnection::streamBodyTo(QIODevice *sink)
{
    m_sink = sink;
}

void HttpConnection::onIdleTimeout()
{
    fail(408);
}

void HttpConnection::onReadyRead()
{
    m_idleTimer->start();

    if (m_state == Responded || m_bodyComplete) {
        // Either we have answered - and we always answer with Connection:
        // close - or the request is complete and waiting on a handler that
        // has not replied yet, which is exactly what a parked prepare-upload
        // looks like. In both cases anything still arriving is a tail we will
        // never serve, and buffering it would grow without bound.
        m_socket->readAll();
        return;
    }

    m_buffer += m_socket->readAll();

    if (m_state == ReadingHeaders) {
        const int end = m_buffer.indexOf("\r\n\r\n");
        if (end < 0) {
            if (m_buffer.size() > MaxHeadSize)
                fail(431);
            return;
        }

        const QByteArray head = m_buffer.left(end);
        m_buffer.remove(0, end + 4);
        if (!parseHead(head))
            return;

        m_state = ReadingBody;

        // Preflight for the browser client, which never gets as far as a body.
        if (m_method == QLatin1String("OPTIONS")) {
            respond(204, QByteArray(), QByteArray());
            return;
        }

        emit headersReady(this);
        if (m_state != ReadingBody)
            return;   // the handler rejected the request before its body

        // Only now, once a handler has accepted it, is the body worth having.
        if (m_headers.value("expect").toLower() == "100-continue")
            m_socket->write("HTTP/1.1 100 Continue\r\n\r\n");
    }

    consumeBody();
}

bool HttpConnection::parseHead(const QByteArray &head)
{
    const QList<QByteArray> lines = head.split('\n');
    if (lines.isEmpty()) {
        fail(400);
        return false;
    }

    const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
    if (requestLine.size() < 2) {
        fail(400);
        return false;
    }

    m_method = QString::fromLatin1(requestLine.at(0)).toUpper();

    const QUrl target = QUrl::fromEncoded(requestLine.at(1), QUrl::StrictMode);
    m_path = target.path();

    const QList<QPair<QString, QString> > items =
            QUrlQuery(target).queryItems(QUrl::FullyDecoded);
    for (int i = 0; i < items.size(); ++i)
        m_query.insert(items.at(i).first, items.at(i).second);

    for (int i = 1; i < lines.size(); ++i) {
        const QByteArray line = lines.at(i).trimmed();
        const int colon = line.indexOf(':');
        if (colon <= 0)
            continue;
        m_headers.insert(line.left(colon).trimmed().toLower(),
                         line.mid(colon + 1).trimmed());
    }

    m_chunked = m_headers.value("transfer-encoding").toLower().contains("chunked");
    if (m_chunked) {
        // Length is unknown until the terminating chunk; progress reporting
        // falls back to "unknown total", which the UI already handles.
        m_contentLength = -1;
    } else {
        bool ok = false;
        const qint64 declared = m_headers.value("content-length").toLongLong(&ok);
        m_contentLength = (ok && declared > 0) ? declared : 0;
    }

    return true;
}

bool HttpConnection::deliver(const QByteArray &data)
{
    if (data.isEmpty())
        return true;

    if (m_sink) {
        if (m_sink->write(data) != data.size()) {
            fail(500);
            return false;
        }
    } else {
        if (m_body.size() + data.size() > MaxBufferedBody) {
            fail(413);
            return false;
        }
        m_body += data;
    }

    m_bodyReceived += data.size();
    emit bodyProgress(m_bodyReceived, m_contentLength);
    return true;
}

void HttpConnection::consumeBody()
{
    if (m_state != ReadingBody || m_bodyComplete)
        return;

    if (m_chunked) {
        consumeChunkedBody();
        return;
    }

    while (m_bodyReceived < m_contentLength && !m_buffer.isEmpty()) {
        const int take = int(qMin<qint64>(m_contentLength - m_bodyReceived,
                                          m_buffer.size()));
        const QByteArray data = m_buffer.left(take);
        m_buffer.remove(0, take);
        if (!deliver(data))
            return;
    }

    if (m_bodyReceived >= m_contentLength)
        finishBody();
}

void HttpConnection::consumeChunkedBody()
{
    for (;;) {
        switch (m_chunkState) {
        case ChunkSize: {
            const int eol = m_buffer.indexOf("\r\n");
            if (eol < 0) {
                if (m_buffer.size() > MaxHeadSize)
                    fail(400);
                return;
            }
            QByteArray sizeLine = m_buffer.left(eol);
            m_buffer.remove(0, eol + 2);

            const int extension = sizeLine.indexOf(';');
            if (extension >= 0)
                sizeLine = sizeLine.left(extension);

            bool ok = false;
            m_chunkRemaining = sizeLine.trimmed().toLongLong(&ok, 16);
            if (!ok || m_chunkRemaining < 0) {
                fail(400);
                return;
            }
            m_chunkState = (m_chunkRemaining == 0) ? ChunkTrailer : ChunkData;
            break;
        }

        case ChunkData: {
            if (m_buffer.isEmpty())
                return;
            const int take = int(qMin<qint64>(m_chunkRemaining, m_buffer.size()));
            const QByteArray data = m_buffer.left(take);
            m_buffer.remove(0, take);
            m_chunkRemaining -= take;
            if (!deliver(data))
                return;
            if (m_chunkRemaining == 0)
                m_chunkState = ChunkCrlf;
            break;
        }

        case ChunkCrlf: {
            if (m_buffer.size() < 2)
                return;
            m_buffer.remove(0, 2);
            m_chunkState = ChunkSize;
            break;
        }

        case ChunkTrailer: {
            // Optional trailer headers followed by a blank line. Nothing we
            // need, so we only look for the terminator.
            const int eol = m_buffer.indexOf("\r\n");
            if (eol < 0)
                return;
            const bool blank = (eol == 0);
            m_buffer.remove(0, eol + 2);
            if (blank) {
                m_chunkState = ChunkDone;
                m_contentLength = m_bodyReceived;
                finishBody();
                return;
            }
            break;
        }

        case ChunkDone:
            return;
        }
    }
}

void HttpConnection::finishBody()
{
    // requestReady() must fire exactly once. A handler that answers straight
    // away is protected by m_state, but prepare-upload deliberately does not
    // answer: it parks the connection while somebody decides. Without this
    // guard, one more byte from the sender would re-deliver the request, and
    // the second delivery would find a session already pending and reject the
    // very transfer that was waiting on it.
    if (m_bodyComplete)
        return;
    m_bodyComplete = true;

    m_idleTimer->stop();
    m_buffer.clear();
    emit requestReady(this);
}

void HttpConnection::fail(int status)
{
    respond(status);
}

void HttpConnection::respond(int status, const QByteArray &payload,
                             const QByteArray &contentType)
{
    if (m_state == Responded)
        return;
    m_state = Responded;
    m_idleTimer->stop();

    QByteArray response;
    response += "HTTP/1.1 " + QByteArray::number(status) + " "
             + reasonPhrase(status) + "\r\n";
    if (!contentType.isEmpty())
        response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + QByteArray::number(payload.size()) + "\r\n";
    for (int i = 0; i < m_extraHeaders.count(); ++i) {
        response += m_extraHeaders.at(i).first + ": "
                 + m_extraHeaders.at(i).second + "\r\n";
    }
    // LocalSend's browser client is cross-origin by construction.
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    response += "Access-Control-Allow-Headers: Content-Type\r\n";
    response += "Connection: close\r\n\r\n";
    response += payload;

    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(response);
        m_socket->flush();
        m_socket->disconnectFromHost();
    }
}

void HttpConnection::addHeader(const QByteArray &name, const QByteArray &value)
{
    m_extraHeaders.append(qMakePair(name, value));
}

void HttpConnection::respondJson(int status, const QJsonObject &object)
{
    respond(status, QJsonDocument(object).toJson(QJsonDocument::Compact));
}

void HttpConnection::onDisconnected()
{
    if (m_closedEmitted)
        return;
    m_closedEmitted = true;
    // Nothing can be written any more, and respond() checks this before it
    // touches the socket.
    m_state = Responded;
    emit closed(this);
    deleteLater();
}

// --- HttpServer ----------------------------------------------------------

HttpServer::HttpServer(QObject *parent)
    : QTcpServer(parent)
    , m_liveConnections(0)
{
}

void HttpServer::setIdentity(const QSslCertificate &certificate, const QSslKey &key)
{
    m_certificate = certificate;
    m_key = key;
}

bool HttpServer::isSecure() const
{
    return !m_certificate.isNull() && !m_key.isNull();
}

int HttpServer::liveConnections() const
{
    return m_liveConnections;
}

bool HttpServer::start(quint16 port)
{
    stop();
    if (!listen(QHostAddress::Any, port)) {
        m_lastError = errorString();
        return false;
    }
    m_lastError.clear();
    return true;
}

void HttpServer::stop()
{
    if (isListening())
        close();
}

bool HttpServer::isListening() const
{
    return QTcpServer::isListening();
}

quint16 HttpServer::boundPort() const
{
    return serverPort();
}

QString HttpServer::lastError() const
{
    return m_lastError;
}

void HttpServer::track(QTcpSocket *socket)
{
    ++m_liveConnections;
    connect(socket, &QObject::destroyed, this, [this]() {
        --m_liveConnections;
    });
}

void HttpServer::adopt(QTcpSocket *socket)
{
    // The connection adopts the socket and outlives this call; it deletes
    // itself once the socket closes.
    HttpConnection *connection = new HttpConnection(socket, this);
    emit connectionReady(connection);
}

void HttpServer::incomingConnection(qintptr socketDescriptor)
{
    if (m_liveConnections >= MaxConnections) {
        // A device that opens more sockets than this is not transferring
        // files, and every one we accept is a file descriptor and a buffer.
        // Refusing costs the peer a retry; accepting costs us the app.
        QTcpSocket refused;
        refused.setSocketDescriptor(socketDescriptor);
        refused.abort();
        return;
    }

    if (!isSecure()) {
        QTcpSocket *socket = new QTcpSocket(this);
        if (!socket->setSocketDescriptor(socketDescriptor)) {
            delete socket;
            return;
        }
        track(socket);
        adopt(socket);
        return;
    }

    QSslSocket *socket = new QSslSocket(this);
    if (!socket->setSocketDescriptor(socketDescriptor)) {
        delete socket;
        return;
    }
    track(socket);

    socket->setLocalCertificate(m_certificate);
    socket->setPrivateKey(m_key);
    socket->setProtocol(QSsl::TlsV1_2OrLater);
    // We do not authenticate senders with certificates - the protocol has no
    // notion of a client identity - so asking for one would only fail
    // handshakes with peers that have none to offer.
    socket->setPeerVerifyMode(QSslSocket::VerifyNone);

    // Nothing may read from the socket until the handshake is done, so the
    // HttpConnection is not built until then.
    connect(socket, &QSslSocket::encrypted, this, [this, socket]() {
        adopt(socket);
    });

    connect(socket, static_cast<void (QSslSocket::*)(const QList<QSslError> &)>(
                        &QSslSocket::sslErrors),
            socket, [socket](const QList<QSslError> &) {
        // Server side with no peer verification: anything reported here is
        // about a client certificate we never asked for.
        socket->ignoreSslErrors();
    });

    connect(socket, static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(
                        &QAbstractSocket::error),
            socket, [socket](QAbstractSocket::SocketError) {
        socket->deleteLater();
    });

    socket->startServerEncryption();

    // A peer that opens a socket and never completes a handshake would
    // otherwise sit against the connection limit indefinitely.
    QTimer::singleShot(HandshakeTimeoutMs, socket, [socket]() {
        if (!socket->isEncrypted())
            socket->abort();
    });
}
