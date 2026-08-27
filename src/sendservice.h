#ifndef SENDSERVICE_H
#define SENDSERVICE_H

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include "deviceinfo.h"

class AppSettings;
class HistoryModel;
class QFile;
class QNetworkAccessManager;
class QNetworkReply;
class TransferModel;

// Drives an outgoing transfer: the prepare-upload handshake, then one file at
// a time until the list runs out.
//
// Files go up sequentially rather than in parallel. It is what the reference
// implementation does, it keeps the per-file progress readable, and on a phone
// it is not slower - a single stream already saturates the radio.
class SendService : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)

public:
    SendService(AppSettings *settings, TransferModel *transfer,
                HistoryModel *history, QNetworkAccessManager *network,
                QObject *parent = 0);

    bool isBusy() const;

    // `device` is a row from DeviceModel::get(). Paths may be plain paths or
    // file:// URLs, because that is what the Silica pickers hand back.
    Q_INVOKABLE void sendFiles(const QVariantMap &device, const QStringList &paths);

    // Retries the handshake with a PIN, after pinRequired() was emitted.
    Q_INVOKABLE void submitPin(const QString &pin);

    Q_INVOKABLE void cancel();

signals:
    void busyChanged();

    // The receiver wants a PIN. The selection and the target are held until
    // submitPin() or cancel().
    void pinRequired(bool retry);

    void declined();
    void finished(const QString &status, int fileCount);

private slots:
    void onPrepareFinished();
    void onUploadFinished();

private:
    void requestUpload(const QString &pin);
    void startNextFile();
    void failTransfer(const QString &message);
    void completeTransfer();
    void closeCurrentFile();
    void writeHistory();
    void notifyPeerCancelled();

    AppSettings *m_settings;
    TransferModel *m_transfer;
    HistoryModel *m_history;
    QNetworkAccessManager *m_network;

    DeviceInfo m_peer;
    QString m_sessionId;

    QPointer<QNetworkReply> m_reply;
    QFile *m_currentFile;
    int m_currentRow;
    bool m_busy;
    bool m_cancelled;
    bool m_pinAttempted;
};

#endif // SENDSERVICE_H
