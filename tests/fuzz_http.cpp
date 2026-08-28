// Feeds arbitrary bytes to the HTTP layer and lets the sanitizers watch.
//
// This is the part of the app a stranger on the network can reach without any
// permission at all: before a token exists, before anybody has tapped Accept,
// the parser has already read whatever arrived. Everything else is guarded by
// something; this is guarded by being correct.
//
// The unit tests cover the shapes somebody thought of. A fuzzer covers the
// ones nobody did - a size line that is 4000 hex digits, a header with no
// value, a chunked body that ends mid-terminator, a request line made of one
// byte repeated. Those are exactly the cases hand-written parsers get wrong.
//
// Driven over a real loopback socket rather than by calling the parser
// directly. That costs execution rate, and buys coverage of the pieces a
// direct call would skip: the read-buffer boundary, the resumable decoder
// splitting an input across passes, the timers, and the teardown path when a
// response goes out mid-body.
//
//   docker compose run --rm fuzz

#include <QCoreApplication>
#include <QBuffer>
#include <QElapsedTimer>
#include <QTcpSocket>

#include "httpserver.h"

namespace {

// Everything below is built once and reused: a QCoreApplication per input
// would spend all the time in setup.
struct Harness
{
    Harness()
        : argc(1)
        , argv{const_cast<char *>("fuzz"), 0}
        , app(argc, argv)
    {
        server.start(0);

        QObject::connect(&server, &HttpServer::connectionReady,
                         [this](HttpConnection *connection) {
            QObject::connect(connection, &HttpConnection::headersReady,
                             [this](HttpConnection *headers) {
                // Mirrors what the receive service does with an upload: the
                // body goes to a sink under a ceiling instead of being
                // buffered. This is the path with the most arithmetic in it.
                if (headers->path().endsWith(QLatin1String("/upload"))) {
                    sink.buffer().clear();
                    sink.open(QIODevice::WriteOnly);
                    headers->streamBodyTo(&sink, 4096);
                }
            });

            QObject::connect(connection, &HttpConnection::requestReady,
                             [](HttpConnection *ready) {
                // Touch the accessors a handler would, so anything that
                // mis-parses into them is exercised rather than ignored.
                ready->jsonBody();
                ready->query(QStringLiteral("sessionId"));
                ready->header("content-type");
                ready->respondJson(200, QJsonObject());
            });
        });
    }

    int argc;
    char *argv[2];
    QCoreApplication app;
    HttpServer server;
    QBuffer sink;
};

Harness &harness()
{
    static Harness instance;
    return instance;
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // A cap keeps the corpus from drifting towards inputs that are slow rather
    // than interesting; the size limits inside the parser are unit-tested.
    if (size == 0 || size > 64 * 1024)
        return 0;

    Harness &h = harness();

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, h.server.boundPort());
    if (!client.waitForConnected(1000))
        return 0;

    client.write(reinterpret_cast<const char *>(data), qint64(size));
    client.flush();

    // Bounded: some inputs deliberately leave the parser waiting for more, and
    // this is not the place to wait out a sixty-second idle timer.
    QElapsedTimer clock;
    clock.start();
    while (clock.elapsed() < 50
           && client.state() != QAbstractSocket::UnconnectedState) {
        h.app.processEvents(QEventLoop::AllEvents, 5);
    }

    client.abort();
    // Let the server finish tearing its side down, so the teardown path is
    // fuzzed too rather than left to the next iteration.
    h.app.processEvents(QEventLoop::AllEvents, 5);

    return 0;
}
