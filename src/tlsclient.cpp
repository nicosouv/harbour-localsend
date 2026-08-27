#include "tlsclient.h"

#include <QDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslConfiguration>
#include <QSslError>

#include "certificate.h"
#include "crypto.h"

namespace TlsClient {

void configure(QNetworkRequest &request)
{
    QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
    configuration.setProtocol(QSsl::TlsV1_2OrLater);

    // VerifyPeer, and not QueryPeer, even though we do the deciding ourselves
    // in the sslErrors handler.
    //
    // The difference is what happens when nobody ignores the errors. Under
    // QueryPeer the certificate is fetched, the errors are reported, and the
    // handshake continues anyway — which turns the fingerprint check into a
    // suggestion: a peer presenting the wrong certificate is still connected
    // to, and the files still go out. Under VerifyPeer an error that is not
    // explicitly ignored aborts the connection, which is what makes the check
    // in pin() actually decide anything.
    configuration.setPeerVerifyMode(QSslSocket::VerifyPeer);
    request.setSslConfiguration(configuration);
}

namespace {

QSslCertificate presentedCertificate(QNetworkReply *reply,
                                     const QList<QSslError> &errors)
{
    const QSslCertificate fromConfiguration =
            reply->sslConfiguration().peerCertificate();
    if (!fromConfiguration.isNull())
        return fromConfiguration;

    // Early in the handshake the configuration may not carry it yet, but the
    // errors themselves name the certificate they are complaining about.
    for (int i = 0; i < errors.count(); ++i) {
        if (!errors.at(i).certificate().isNull())
            return errors.at(i).certificate();
    }

    return QSslCertificate();
}

} // namespace

void pin(QNetworkReply *reply, const QString &expectedFingerprint)
{
    if (!reply)
        return;

    QObject::connect(reply, &QNetworkReply::sslErrors, reply,
                     [reply, expectedFingerprint](const QList<QSslError> &errors) {
        if (expectedFingerprint.isEmpty()) {
            // Nothing to check against, so nothing is acceptable.
            qWarning("localsend: refusing %s, no fingerprint to pin against",
                     qPrintable(reply->url().host()));
            return;
        }

        const QSslCertificate presented = presentedCertificate(reply, errors);
        if (presented.isNull()) {
            qWarning("localsend: refusing %s, it presented no certificate",
                     qPrintable(reply->url().host()));
            return;
        }

        // Case-folded: the protocol text does not fix the casing of the hex,
        // and implementations differ. Comparing byte-exact makes every peer
        // that chose the other convention unreachable, and the symptom is a
        // bare network error that points nowhere near the cause.
        const QString observed = Certificate::fingerprintOf(presented);
        if (!Crypto::equalsFold(observed, expectedFingerprint)) {
            // Logged in full because this is the failure that otherwise
            // surfaces as a bare "could not reach": the handshake is aborted
            // here, the request never leaves, and nothing downstream knows
            // why. Fingerprints are public values, announced to the whole
            // network, so there is nothing to protect by hiding them.
            qWarning("localsend: refusing %s, certificate fingerprint %s "
                     "does not match the announced %s",
                     qPrintable(reply->url().host()),
                     qPrintable(observed), qPrintable(expectedFingerprint));
            return;
        }

        // The key is the one that announced itself. Everything X.509 objects
        // to from here - self-signed, unknown issuer, wrong host name, an
        // expiry set by a phone with a wrong clock - is about a trust model
        // that does not apply.
        reply->ignoreSslErrors(errors);
    });
}

void acceptUnknown(QNetworkReply *reply)
{
    if (!reply)
        return;

    QObject::connect(reply, &QNetworkReply::sslErrors, reply,
                     [reply](const QList<QSslError> &errors) {
        reply->ignoreSslErrors(errors);
    });
}

QString observedFingerprint(QNetworkReply *reply)
{
    if (!reply)
        return QString();
    return Certificate::fingerprintOf(reply->sslConfiguration().peerCertificate());
}

} // namespace TlsClient
