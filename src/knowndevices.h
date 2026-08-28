#ifndef KNOWNDEVICES_H
#define KNOWNDEVICES_H

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QString>

// Remembers which key belongs to which name, so impersonation becomes visible.
//
// Certificate pinning proves that the device we connect to is the one that
// announced itself. It proves nothing about whether that device is who it
// claims to be, because the announcement is an unauthenticated multicast
// packet: anyone on the network can send one carrying any alias and their own
// fingerprint. Without something remembering what came before, "MacBook" in
// the list is only ever a string somebody chose.
//
// This is that memory. A pairing is recorded after a transfer actually
// completes - not when a device is merely seen - because recording on
// discovery would let an attacker poison the store simply by announcing
// first. From then on, a name we have exchanged files with arriving under a
// different key is reported as a conflict rather than shown as the same
// device.
//
// It is trust on first use, with the limits that implies: the first exchange
// is unverified, and the honest thing to do about it is to make the
// fingerprint easy to compare out of band, which the device details page does.
class KnownDevices : public QObject
{
    Q_OBJECT

public:
    explicit KnownDevices(QObject *parent = 0);

    bool isKnown(const QString &fingerprint) const;
    QString aliasFor(const QString &fingerprint) const;
    QDateTime firstSeen(const QString &fingerprint) const;

    // True when `alias` is already recorded under a different fingerprint.
    // Somebody is using a name we know with a key we have never seen.
    Q_INVOKABLE bool conflicts(const QString &fingerprint,
                               const QString &alias) const;

    // The fingerprint we have on record for `alias`, for telling the user
    // what changed.
    Q_INVOKABLE QString expectedFingerprint(const QString &alias) const;

    // Records the pairing. Called after a completed exchange, never on
    // discovery.
    void remember(const QString &fingerprint, const QString &alias);

    Q_INVOKABLE void forget(const QString &fingerprint);
    Q_INVOKABLE void forgetAll();

    int count() const;

signals:
    void changed();

private:
    struct Entry
    {
        QString alias;
        QDateTime firstSeen;
    };

    QString storagePath() const;
    void load();
    void save();

    QHash<QString, Entry> m_entries;   // fingerprint -> entry
};

#endif // KNOWNDEVICES_H
