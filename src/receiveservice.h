#ifndef RECEIVESERVICE_H
#define RECEIVESERVICE_H

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>

#include "deviceinfo.h"
#include "ratelimiter.h"

class AppSettings;
class Discovery;
class HistoryModel;
class HttpConnection;
class HttpServer;
class QFile;
class QNetworkAccessManager;
class QTimer;
class TransferModel;

// Serves every endpoint a sender talks to, and owns the incoming session.
//
// The shape of prepare-upload is what drives the design: the sender opens a
// connection and holds it open until we answer, and our answer is whatever
// the person decides. So that connection is parked - not answered, not
// dropped - while a page goes up asking them, and the reply is written from
// accept() or decline() minutes later.
class ReceiveService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool listening READ isListening NOTIFY listeningChanged)
    Q_PROPERTY(QString listenError READ listenError NOTIFY listeningChanged)
    Q_PROPERTY(int port READ port NOTIFY listeningChanged)

public:
    ReceiveService(AppSettings *settings, Discovery *discovery,
                   TransferModel *transfer, HistoryModel *history,
                   QNetworkAccessManager *network, QObject *parent = 0);

    bool isListening() const;
    QString listenError() const;
    int port() const;

    Q_INVOKABLE bool startListening();
    Q_INVOKABLE void stopListening();

    // Answers the parked prepare-upload. Both are no-ops once the sender has
    // given up and closed the connection, which is the common case for a
    // request that sat on a locked screen.
    Q_INVOKABLE void accept();
    Q_INVOKABLE void decline();

    // Stops an accepted transfer part-way and removes the partial files.
    Q_INVOKABLE void cancel();

signals:
    void listeningChanged();
    // Parameter names cross into QML as the handler's arguments, so they are
    // named for what a page would want to call them.
    void requestArrived(const QString &peerAlias, int fileCount, qint64 totalBytes);
    void transferStarted();
    void transferFinished(const QString &status, int fileCount, const QString &destination);

private slots:
    void onConnectionReady(HttpConnection *connection);
    void onHeadersReady(HttpConnection *connection);
    void onRequestReady(HttpConnection *connection);
    void onPendingClosed(HttpConnection *connection);
    void onUploadClosed(HttpConnection *connection);
    void onAcceptTimeout();
    void onSessionIdle();
    void onSettingsChanged();

private:
    // One file being written. Keyed by connection rather than kept as a single
    // "current" file, so a sender that uploads two files at once cannot make
    // us write both into the same handle.
    struct Upload
    {
        int row;
        QFile *file;
        QString partPath;
        QString finalPath;

        Upload();
    };

    void handleInfo(HttpConnection *connection);
    void handleRegister(HttpConnection *connection);
    void handlePrepareUpload(HttpConnection *connection);
    void handleUploadHeaders(HttpConnection *connection);
    void handleUploadFinished(HttpConnection *connection);
    void handleCancel(HttpConnection *connection);

    QString endpoint(const char *path) const;
    void releasePending();
    void closeUpload(HttpConnection *connection, bool keepFile);
    void finishSession(const QString &status);
    void notifyPeerCancelled();

    QString prepareDirectory();
    QString reserveFilePath(const QString &directory, const QString &fileName) const;
    static QString sanitizeFileName(const QString &name);

    AppSettings *m_settings;
    Discovery *m_discovery;
    TransferModel *m_transfer;
    HistoryModel *m_history;
    QNetworkAccessManager *m_network;
    HttpServer *m_server;

    // Guarded rather than raw: the connection deletes itself the instant its
    // socket closes, which can happen between two lines of a handler.
    QPointer<HttpConnection> m_pending;
    DeviceInfo m_peer;

    QString m_sessionId;
    QString m_sessionAddress;
    QString m_sessionDirectory;
    QHash<QString, QString> m_tokens;      // fileId -> token
    QHash<HttpConnection *, Upload> m_uploads;

    QTimer *m_acceptTimer;
    QTimer *m_idleTimer;
    QString m_listenError;

    // Per-address backoff on wrong PINs. Deliberately not persisted: a
    // restart clearing it costs an attacker a reboot of somebody else's
    // phone, which is not a shortcut worth defending against, and persisting
    // it would mean writing to disk on every failed guess.
    RateLimiter m_pinAttempts;
};

#endif // RECEIVESERVICE_H
