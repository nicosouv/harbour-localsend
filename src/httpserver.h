#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QByteArray>
#include <QHash>
#include <QHostAddress>
#include <QList>
#include <QPair>
#include <QJsonObject>
#include <QObject>
#include <QSslCertificate>
#include <QSslKey>
#include <QString>
#include <QTcpServer>

class QIODevice;
class QTcpSocket;
class QTimer;

// One in-flight request. Qt 5.6 has no HTTP server, so this is the whole of
// our HTTP/1.1: a request line, headers, and a body that is either buffered
// (small JSON) or streamed straight to disk (a file upload, which can be
// gigabytes and must never be held in memory).
//
// The object lives as long as its socket. A holder that keeps a connection
// past the handler - prepare-upload parks one while the user decides - must
// connect to closed() and drop its pointer there.
class HttpConnection : public QObject
{
    Q_OBJECT

public:
    explicit HttpConnection(QTcpSocket *socket, QObject *parent = 0);
    ~HttpConnection();

    QString method() const;
    QString path() const;
    QString query(const QString &key) const;
    bool hasQuery(const QString &key) const;
    QByteArray header(const QByteArray &name) const;

    QByteArray body() const;
    QJsonObject jsonBody() const;

    QString peerAddress() const;
    // The certificate the client presented, null over plain HTTP or when it
    // offered none. Captured at construction, which is after the handshake.
    QSslCertificate peerCertificate() const;
    qint64 contentLength() const;
    qint64 bodyReceived() const;

    // Push the body into `sink` instead of buffering it. Only legal from a
    // headersReady() handler: by requestReady() the body is already gone.
    // Ownership of `sink` stays with the caller.
    //
    // `maxBytes` is a hard ceiling, not a hint: a peer states a file's size
    // when it asks permission and then sends the bytes in a separate request,
    // and nothing but this makes the second agree with the first. Without it
    // an accepted transfer of one small photo can keep writing until the disk
    // is full. -1 means no ceiling, which is only right for a body we are
    // buffering and have already capped another way.
    void streamBodyTo(QIODevice *sink, qint64 maxBytes = -1);

    // Adds a header to whatever response comes next. Used for the handful of
    // cases where the status code alone is not enough - Retry-After on a 429
    // being the one that matters, since a client without it can only guess.
    // Header values are stripped of CR and LF. Nothing peer-controlled goes
    // through here today, but a response is assembled by concatenation and
    // one embedded newline turns a header into a forged second response.
    void addHeader(const QByteArray &name, const QByteArray &value);

    void respond(int status, const QByteArray &payload = QByteArray(),
                 const QByteArray &contentType = QByteArray("application/json"));
    void respondJson(int status, const QJsonObject &object);
    bool hasResponded() const;

signals:
    // Request line and headers parsed, no body consumed yet. The only place
    // streamBodyTo() may be called, and the place to reject a request whose
    // body we do not want to read at all.
    void headersReady(HttpConnection *connection);

    // The whole body has arrived, buffered or streamed.
    void requestReady(HttpConnection *connection);

    void bodyProgress(qint64 received, qint64 total);

    // The socket is gone. Fired exactly once, before deleteLater().
    void closed(HttpConnection *connection);

private slots:
    void onReadyRead();
    void onDisconnected();
    void onIdleTimeout();
    void onHeaderDeadline();

private:
    enum State { ReadingHeaders, ReadingBody, Responded };

    // Chunked bodies arrive in pieces that can split anywhere, including in
    // the middle of a size line, so decoding has to be resumable.
    enum ChunkState { ChunkSize, ChunkData, ChunkCrlf, ChunkTrailer, ChunkDone };

    bool parseHead(const QByteArray &head);
    // Hands decoded body bytes to the sink or the buffer. False once it has
    // failed the request, at which point the caller must stop.
    bool deliver(const QByteArray &data);
    void consumeBody();
    void consumeChunkedBody();
    void finishBody();
    void fail(int status);

    QTcpSocket *m_socket;
    QTimer *m_idleTimer;
    // An absolute deadline for getting the request line and headers in. The
    // idle timer restarts on every byte, so on its own a peer sending one
    // character a minute holds a connection slot for as long as it likes.
    QTimer *m_headerDeadline;
    State m_state;

    QByteArray m_buffer;
    QString m_method;
    QString m_path;
    QHash<QString, QString> m_query;
    QHash<QByteArray, QByteArray> m_headers;
    QList<QPair<QByteArray, QByteArray> > m_extraHeaders;

    QByteArray m_body;
    QIODevice *m_sink;
    qint64 m_maxBodyBytes;        // -1 for no ceiling
    qint64 m_contentLength;       // -1 while a chunked body is still arriving
    qint64 m_bodyReceived;
    bool m_chunked;
    ChunkState m_chunkState;
    qint64 m_chunkRemaining;
    qint64 m_trailerBytes;
    // Set once requestReady() has been emitted, whether or not a handler has
    // answered yet.
    bool m_bodyComplete;

    QString m_peerAddress;
    QSslCertificate m_peerCertificate;
    bool m_closedEmitted;
};

// Accepts connections and hands each one out as an HttpConnection. Routing is
// deliberately not its job: the receive service owns every endpoint and knows
// which ones need a body on disk.
class HttpServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit HttpServer(QObject *parent = 0);

    // Serves TLS with this key pair. Must be set before start(); passing a
    // null certificate puts the server back into plain HTTP.
    void setIdentity(const QSslCertificate &certificate, const QSslKey &key);
    bool isSecure() const;

    // Binds every interface on `port`. Returns false and sets lastError() when
    // the port is taken, which on this platform usually means a second copy of
    // the app is already listening.
    bool start(quint16 port);
    void stop();

    bool isListening() const;
    quint16 boundPort() const;
    QString lastError() const;

    int liveConnections() const;

signals:
    void connectionReady(HttpConnection *connection);

protected:
    void incomingConnection(qintptr socketDescriptor);

private:
    void adopt(QTcpSocket *socket);
    void track(QTcpSocket *socket);

    QString m_lastError;
    QSslCertificate m_certificate;
    QSslKey m_key;
    int m_liveConnections;
};

#endif // HTTPSERVER_H
