#ifndef CERTIFICATE_H
#define CERTIFICATE_H

#include <QSslCertificate>
#include <QSslKey>
#include <QString>

// This device's TLS identity: a self-signed certificate generated once and
// kept forever.
//
// Every LocalSend device is its own certificate authority — there is no PKI on
// a living-room network and there never will be. What replaces it is the
// fingerprint: in HTTPS mode the protocol defines it as the SHA-256 of the
// certificate, it travels in every announcement, and a peer that presents a
// certificate hashing to something else is not the device that announced
// itself. That is the whole security model, and it is why this class owns
// both the key pair and the fingerprint rather than leaving them in separate
// places that could disagree.
//
// Qt cannot generate a certificate — no version of it ever could — so this is
// the one place that talks to OpenSSL directly.
class Certificate
{
public:
    Certificate();

    // Loads the stored pair, generating one on first run. False means we have
    // no usable identity and the caller must fall back to plain HTTP; the
    // reason is in lastError().
    bool ensure(const QString &directory);

    bool isValid() const;

    QSslCertificate certificate() const;
    QSslKey privateKey() const;

    // SHA-256 of the DER form, lowercase hex.
    QString fingerprint() const;

    QString lastError() const;

    // The fingerprint of somebody else's certificate, in the same form, for
    // comparing against what a peer announced.
    static QString fingerprintOf(const QSslCertificate &certificate);

private:
    bool load(const QString &certPath, const QString &keyPath);
    bool generate(const QString &certPath, const QString &keyPath);

    QSslCertificate m_certificate;
    QSslKey m_key;
    QString m_fingerprint;
    QString m_lastError;
};

#endif // CERTIFICATE_H
