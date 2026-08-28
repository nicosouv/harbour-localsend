#include <QBuffer>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QSignalSpy>
#include <QTcpSocket>
#include <QtTest>

#include "httpserver.h"

// The HTTP layer, driven by raw bytes on a real socket. Everything a peer can
// legally send is here because there is no library underneath to get it right
// for us: this file *is* the HTTP implementation's specification.
class TestHttpServer : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void parsesMethodPathAndQuery();
    void bufferesABodyWithContentLength();
    void decodesAChunkedBody();
    void streamsABodyToASink();
    void answersPreflightWithoutABody();
    void rejectsBeforeReadingTheBody();
    void reportsProgressWhileStreaming();
    void deliversAParkedRequestOnlyOnce();
    void refusesAHeaderThatContradictsItself();
    void refusesAnEndlessChunkTrailer();
    void keepsInjectedNewlinesOutOfHeaders();

private:
    // Sends a raw request and returns the whole response.
    QByteArray exchange(const QByteArray &request, int timeoutMs = 3000);

    HttpServer *m_server;
};

void TestHttpServer::init()
{
    m_server = new HttpServer(this);
    // Port 0 lets the kernel pick, so tests never collide with a real app.
    QVERIFY(m_server->start(0));
}

void TestHttpServer::cleanup()
{
    m_server->stop();
    delete m_server;
    m_server = 0;
}

QByteArray TestHttpServer::exchange(const QByteArray &request, int timeoutMs)
{
    QTcpSocket socket;
    QByteArray response;
    bool finished = false;

    // Both ends live in this thread, so the client must never block: a
    // waitForReadyRead() here would stop the event loop the server needs to
    // notice the connection at all, and every exchange would time out.
    connect(&socket, &QTcpSocket::readyRead, this, [&socket, &response, &finished]() {
        response += socket.readAll();

        // Stop on a complete response rather than only on disconnect. A
        // request refused mid-body is answered and then drained, so the
        // server deliberately holds the socket open for a moment - waiting
        // for the close would make every such test sit out that deadline.
        const int headerEnd = response.indexOf("\r\n\r\n");
        if (headerEnd < 0)
            return;

        const QByteArray head = response.left(headerEnd).toLower();
        const int marker = head.indexOf("content-length:");
        if (marker < 0)
            return;

        const int declared =
            head.mid(marker + 15, head.indexOf('\r', marker) - marker - 15).trimmed().toInt();
        if (response.size() - headerEnd - 4 >= declared)
            finished = true;
    });
    connect(&socket, &QTcpSocket::disconnected, this, [&finished]() {
        finished = true;
    });

    socket.connectToHost(QHostAddress::LocalHost, m_server->boundPort());
    if (!socket.waitForConnected(timeoutMs))
        return QByteArray();

    socket.write(request);
    socket.flush();

    QElapsedTimer clock;
    clock.start();
    while (!finished && clock.elapsed() < timeoutMs)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);

    response += socket.readAll();
    return response;
}

void TestHttpServer::parsesMethodPathAndQuery()
{
    // Read out of the handler rather than off a stored pointer: the
    // connection deletes itself the moment its socket closes, which is
    // before this function gets to look at anything.
    QString method;
    QString path;
    QString sessionId;
    QString fileId;
    QByteArray custom;
    bool sawMissingQuery = true;

    connect(m_server, &HttpServer::connectionReady, this,
            [&](HttpConnection *connection) {
        connect(connection, &HttpConnection::requestReady, this,
                [&](HttpConnection *ready) {
            method = ready->method();
            path = ready->path();
            sessionId = ready->query(QStringLiteral("sessionId"));
            fileId = ready->query(QStringLiteral("fileId"));
            custom = ready->header("x-custom");
            sawMissingQuery = ready->hasQuery(QStringLiteral("missing"));
            ready->respond(200, "{\"ok\":true}");
        });
    });

    const QByteArray response = exchange(
        "GET /api/localsend/v2/upload?sessionId=s1&fileId=f%201&token=t1 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "X-Custom: hello\r\n"
        "\r\n");

    QVERIFY(response.startsWith("HTTP/1.1 200 OK"));
    QVERIFY(response.contains("{\"ok\":true}"));
    QVERIFY(response.contains("Content-Length: 11"));

    QCOMPARE(method, QStringLiteral("GET"));
    QCOMPARE(path, QStringLiteral("/api/localsend/v2/upload"));
    QCOMPARE(sessionId, QStringLiteral("s1"));
    // Percent-encoding has to be undone: file names routinely contain spaces.
    QCOMPARE(fileId, QStringLiteral("f 1"));
    QCOMPARE(custom, QByteArray("hello"));
    QVERIFY(!sawMissingQuery);
}

void TestHttpServer::bufferesABodyWithContentLength()
{
    QByteArray seen;
    connect(m_server, &HttpServer::connectionReady, this,
            [this, &seen](HttpConnection *connection) {
        connect(connection, &HttpConnection::requestReady, this,
                [&seen](HttpConnection *ready) {
            seen = ready->body();
            ready->respond(200);
        });
    });

    const QByteArray body = "{\"info\":{\"alias\":\"Test\"}}";
    QByteArray request = "POST /api/localsend/v2/prepare-upload HTTP/1.1\r\n"
                         "Content-Type: application/json\r\n"
                         "Content-Length: " + QByteArray::number(body.size()) + "\r\n"
                         "\r\n" + body;

    const QByteArray response = exchange(request);
    QVERIFY(response.startsWith("HTTP/1.1 200 OK"));
    QCOMPARE(seen, body);
}

void TestHttpServer::decodesAChunkedBody()
{
    // A sender that streams without knowing the length up front uses chunked
    // encoding, and a receiver that ignores it writes the framing into the file.
    QByteArray seen;
    connect(m_server, &HttpServer::connectionReady, this,
            [this, &seen](HttpConnection *connection) {
        connect(connection, &HttpConnection::requestReady, this,
                [&seen](HttpConnection *ready) {
            seen = ready->body();
            ready->respond(200);
        });
    });

    const QByteArray response = exchange(
        "POST /api/localsend/v2/prepare-upload HTTP/1.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\nHello\r\n"
        "1\r\n \r\n"
        "6\r\nworld!\r\n"
        "0\r\n"
        "\r\n");

    QVERIFY(response.startsWith("HTTP/1.1 200 OK"));
    QCOMPARE(seen, QByteArray("Hello world!"));
}

void TestHttpServer::streamsABodyToASink()
{
    QBuffer sink;
    sink.open(QIODevice::WriteOnly);

    connect(m_server, &HttpServer::connectionReady, this,
            [this, &sink](HttpConnection *connection) {
        connect(connection, &HttpConnection::headersReady, this,
                [&sink](HttpConnection *headers) {
            headers->streamBodyTo(&sink);
        });
        connect(connection, &HttpConnection::requestReady, this,
                [](HttpConnection *ready) {
            ready->respond(200);
        });
    });

    const QByteArray payload(200000, 'x');
    QByteArray request = "POST /api/localsend/v2/upload HTTP/1.1\r\n"
                         "Content-Length: " + QByteArray::number(payload.size()) + "\r\n"
                         "\r\n" + payload;

    const QByteArray response = exchange(request, 8000);
    QVERIFY(response.startsWith("HTTP/1.1 200 OK"));
    QCOMPARE(sink.buffer().size(), payload.size());
    QCOMPARE(sink.buffer(), payload);
}

void TestHttpServer::answersPreflightWithoutABody()
{
    bool sawRequest = false;
    connect(m_server, &HttpServer::connectionReady, this,
            [this, &sawRequest](HttpConnection *connection) {
        connect(connection, &HttpConnection::requestReady, this,
                [&sawRequest](HttpConnection *ready) {
            sawRequest = true;
            ready->respond(500);
        });
    });

    const QByteArray response = exchange(
        "OPTIONS /api/localsend/v2/prepare-upload HTTP/1.1\r\n"
        "Origin: http://localsend.org\r\n"
        "\r\n");

    QVERIFY(response.startsWith("HTTP/1.1 204"));
    QVERIFY(response.contains("Access-Control-Allow-Origin: *"));
    // The router must never see a preflight, or every endpoint would have to
    // special-case it.
    QVERIFY(!sawRequest);
}

void TestHttpServer::rejectsBeforeReadingTheBody()
{
    // The point of headersReady: a token that does not check out must be
    // refused without a gigabyte of upload landing on the disk first.
    bool bodyArrived = false;
    connect(m_server, &HttpServer::connectionReady, this,
            [this, &bodyArrived](HttpConnection *connection) {
        connect(connection, &HttpConnection::headersReady, this,
                [](HttpConnection *headers) {
            headers->respond(403);
        });
        connect(connection, &HttpConnection::requestReady, this,
                [&bodyArrived](HttpConnection *) {
            bodyArrived = true;
        });
    });

    const QByteArray body(50000, 'y');
    QByteArray request = "POST /api/localsend/v2/upload HTTP/1.1\r\n"
                         "Content-Length: " + QByteArray::number(body.size()) + "\r\n"
                         "\r\n" + body;

    const QByteArray response = exchange(request);
    QVERIFY(response.startsWith("HTTP/1.1 403"));
    QVERIFY(!bodyArrived);
}

void TestHttpServer::reportsProgressWhileStreaming()
{
    QBuffer sink;
    sink.open(QIODevice::WriteOnly);

    QList<qint64> progress;
    qint64 announcedTotal = -1;

    connect(m_server, &HttpServer::connectionReady, this,
            [this, &sink, &progress, &announcedTotal](HttpConnection *connection) {
        connect(connection, &HttpConnection::headersReady, this,
                [&sink](HttpConnection *headers) {
            headers->streamBodyTo(&sink);
        });
        connect(connection, &HttpConnection::bodyProgress, this,
                [&progress, &announcedTotal](qint64 received, qint64 total) {
            progress.append(received);
            announcedTotal = total;
        });
        connect(connection, &HttpConnection::requestReady, this,
                [](HttpConnection *ready) { ready->respond(200); });
    });

    const QByteArray payload(400000, 'z');
    QByteArray request = "POST /api/localsend/v2/upload HTTP/1.1\r\n"
                         "Content-Length: " + QByteArray::number(payload.size()) + "\r\n"
                         "\r\n" + payload;

    exchange(request, 8000);

    QVERIFY(!progress.isEmpty());
    QCOMPARE(announcedTotal, qint64(payload.size()));
    QCOMPARE(progress.last(), qint64(payload.size()));

    // Counts are absolute and monotonic: the UI adds nothing up itself.
    for (int i = 1; i < progress.count(); ++i)
        QVERIFY(progress.at(i) > progress.at(i - 1));
}

void TestHttpServer::deliversAParkedRequestOnlyOnce()
{
    // prepare-upload is answered minutes later, or never. Between the request
    // arriving and the answer going out the connection sits there complete
    // and unanswered, and anything else the sender writes must be ignored.
    // Re-delivering it would hand the receive service a second request while
    // the first is still pending, and it would reject it with 409 - refusing
    // the very transfer that was waiting on the person to say yes.
    int deliveries = 0;

    connect(m_server, &HttpServer::connectionReady, this,
            [this, &deliveries](HttpConnection *connection) {
        connect(connection, &HttpConnection::requestReady, this,
                [&deliveries](HttpConnection *) {
            ++deliveries;
            // No response, on purpose.
        });
    });

    QTcpSocket socket;
    socket.connectToHost(QHostAddress::LocalHost, m_server->boundPort());
    QVERIFY(socket.waitForConnected(3000));

    socket.write("POST /api/localsend/v2/prepare-upload HTTP/1.1\r\n"
                 "Content-Length: 2\r\n"
                 "\r\n"
                 "{}");
    socket.flush();

    QElapsedTimer clock;
    clock.start();
    while (deliveries == 0 && clock.elapsed() < 3000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);

    QCOMPARE(deliveries, 1);

    socket.write("\r\n");
    socket.flush();

    clock.restart();
    while (clock.elapsed() < 400)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);

    QCOMPARE(deliveries, 1);
}

void TestHttpServer::refusesAHeaderThatContradictsItself()
{
    // Two Content-Lengths are two answers to where the body ends. Picking one
    // is the ambiguity request smuggling is built on, so neither is picked.
    bool reached = false;
    connect(m_server, &HttpServer::connectionReady, this,
            [this, &reached](HttpConnection *connection) {
        connect(connection, &HttpConnection::requestReady, this,
                [&reached](HttpConnection *ready) {
            reached = true;
            ready->respond(200);
        });
    });

    const QByteArray response = exchange(
        "POST /api/localsend/v2/prepare-upload HTTP/1.1\r\n"
        "Content-Length: 5\r\n"
        "Content-Length: 500\r\n"
        "\r\n"
        "hello");

    QVERIFY(response.startsWith("HTTP/1.1 400"));
    QVERIFY(!reached);
}

void TestHttpServer::refusesAnEndlessChunkTrailer()
{
    // Trailers never reach deliver(), so nothing else bounds them, and every
    // line read resets the idle timer. A peer streaming them forever would
    // hold a connection slot forever without ever completing a request.
    connect(m_server, &HttpServer::connectionReady, this,
            [this](HttpConnection *connection) {
        connect(connection, &HttpConnection::requestReady, this,
                [](HttpConnection *ready) { ready->respond(200); });
    });

    QByteArray request =
        "POST /api/localsend/v2/prepare-upload HTTP/1.1\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "2\r\nhi\r\n"
        "0\r\n";
    // Well past the header budget, and never the blank line that ends them.
    for (int i = 0; i < 2000; ++i)
        request += "X-Padding: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\r\n";

    const QByteArray response = exchange(request, 8000);
    QVERIFY(response.startsWith("HTTP/1.1 431"));
}

void TestHttpServer::keepsInjectedNewlinesOutOfHeaders()
{
    // A response is assembled by concatenation, so one embedded newline in a
    // header value would end the header block early and let the rest of the
    // value be read as a second, forged response.
    connect(m_server, &HttpServer::connectionReady, this,
            [this](HttpConnection *connection) {
        connect(connection, &HttpConnection::requestReady, this,
                [](HttpConnection *ready) {
            ready->addHeader("Retry-After", "5\r\nX-Injected: yes");
            ready->respond(429);
        });
    });

    const QByteArray response = exchange(
        "GET /api/localsend/v2/info HTTP/1.1\r\n\r\n");

    QVERIFY(response.startsWith("HTTP/1.1 429"));

    // The text survives, flattened onto one line - what must not survive is a
    // line break in front of it, which is what would have turned the tail
    // into a header of its own.
    QVERIFY(!response.contains("\r\nX-Injected"));
    QVERIFY(response.contains("Retry-After: 5X-Injected: yes"));
}

QTEST_MAIN(TestHttpServer)
#include "tst_httpserver.moc"
