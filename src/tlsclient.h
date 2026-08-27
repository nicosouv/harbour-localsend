#ifndef TLSCLIENT_H
#define TLSCLIENT_H

#include <QString>

class QNetworkReply;
class QNetworkRequest;

// How we talk TLS to a peer that signed its own certificate.
//
// There is no certificate authority on a home network, so the usual chain
// check has nothing to say: every device here is its own root, and a chain
// that validates would be the surprising outcome. What takes its place is the
// fingerprint. A device announces the SHA-256 of its certificate over
// multicast; when we then connect and are handed a certificate, hashing it
// and comparing tells us whether this is the device that announced itself or
// somebody who answered in its place.
//
// So the rule below is deliberately blunt: if the fingerprint matches, accept
// the connection whatever X.509 thinks of it - expiry, host name and issuer
// are all meaningless for a self-signed peer. If it does not match, accept
// nothing. Anything in between would be pretending to a kind of trust that
// does not exist here.
namespace TlsClient {

// TLS 1.2 or better on an outgoing request.
void configure(QNetworkRequest &request);

// Accepts the connection only when the presented certificate hashes to
// `expectedFingerprint`. An empty fingerprint pins against nothing and so
// refuses everything, which is the safe reading of "we do not know who this
// is".
void pin(QNetworkReply *reply, const QString &expectedFingerprint);

// For discovery only, where by definition we have not met the device yet and
// have no fingerprint to pin against. The certificate is accepted so the
// exchange can happen, and the caller must then check that the fingerprint in
// the reply body matches observedFingerprint() before believing a word of it.
//
// Never use this for a transfer: it establishes who a device claims to be,
// not that the claim is worth anything.
void acceptUnknown(QNetworkReply *reply);

// The hash of the certificate the peer actually presented, or an empty string
// for a plain-HTTP exchange.
QString observedFingerprint(QNetworkReply *reply);

} // namespace TlsClient

#endif // TLSCLIENT_H
