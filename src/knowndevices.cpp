#include "knowndevices.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QVariantMap>

#include "crypto.h"
#include "securestore.h"

KnownDevices::Entry::Entry()
    : paired(false)
    , blocked(false)
{
}

KnownDevices::KnownDevices(QObject *parent)
    : QObject(parent)
{
    load();
}

QString KnownDevices::storagePath() const
{
    const QString directory =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(directory);
    return directory + QStringLiteral("/known-devices.json");
}

void KnownDevices::load()
{
    const QJsonDocument document = QJsonDocument::fromJson(
        SecureStore::instance().read(storagePath()));
    if (!document.isObject())
        return;

    const QJsonObject root = document.object();
    const QStringList fingerprints = root.keys();

    for (int i = 0; i < fingerprints.count(); ++i) {
        const QJsonObject value = root.value(fingerprints.at(i)).toObject();

        Entry entry;
        entry.alias = value.value(QStringLiteral("alias")).toString();
        entry.firstSeen = QDateTime::fromMSecsSinceEpoch(
            qint64(value.value(QStringLiteral("firstSeen")).toDouble()));
        entry.blocked = value.value(QStringLiteral("blocked")).toBool(false);
        // Catalogues written before blocking existed hold nothing but
        // completed pairings, so an entry with no flag and no block is one.
        entry.paired = value.value(QStringLiteral("paired")).toBool(!entry.blocked);

        // Stored uppercase so a catalogue written by an older build, or by a
        // peer using the other hex convention, still matches on lookup.
        m_entries.insert(fingerprints.at(i).toUpper(), entry);
    }
}

void KnownDevices::save()
{
    QJsonObject root;

    QHash<QString, Entry>::const_iterator it = m_entries.constBegin();
    for (; it != m_entries.constEnd(); ++it) {
        QJsonObject value;
        value.insert(QStringLiteral("alias"), it.value().alias);
        value.insert(QStringLiteral("firstSeen"),
                     double(it.value().firstSeen.toMSecsSinceEpoch()));
        value.insert(QStringLiteral("paired"), it.value().paired);
        value.insert(QStringLiteral("blocked"), it.value().blocked);
        root.insert(it.key(), value);
    }

    // Authenticated encryption matters here beyond confidentiality: this file
    // decides which key is allowed to wear which name, so an attacker able to
    // edit it could silently retire the impersonation warning.
    SecureStore::instance().write(storagePath(),
                                  QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool KnownDevices::isBlocked(const QString &fingerprint) const
{
    if (fingerprint.isEmpty())
        return false;
    return m_entries.value(fingerprint.toUpper()).blocked;
}

void KnownDevices::setBlocked(const QString &fingerprint, const QString &alias,
                              bool blocked)
{
    if (fingerprint.isEmpty())
        return;

    const QString key = fingerprint.toUpper();
    QHash<QString, Entry>::iterator it = m_entries.find(key);

    if (it == m_entries.end()) {
        if (!blocked)
            return;   // nothing to unblock

        // Blocking a device we have never exchanged with is the common case:
        // it is exactly the peer nobody wants to hear from again.
        Entry entry;
        entry.alias = alias;
        entry.firstSeen = QDateTime::currentDateTime();
        entry.blocked = true;
        m_entries.insert(key, entry);
    } else {
        if (it.value().blocked == blocked)
            return;
        it.value().blocked = blocked;
        if (!alias.isEmpty())
            it.value().alias = alias;
    }

    save();
    emit changed();
}

QVariantList KnownDevices::blocked() const
{
    QVariantList list;

    QHash<QString, Entry>::const_iterator it = m_entries.constBegin();
    for (; it != m_entries.constEnd(); ++it) {
        if (!it.value().blocked)
            continue;
        QVariantMap entry;
        entry.insert(QStringLiteral("fingerprint"), it.key());
        entry.insert(QStringLiteral("alias"), it.value().alias);
        list.append(entry);
    }
    return list;
}

// "Known" means we have exchanged files with this key, not that it appears in
// the file: a key that is only there because somebody blocked it has never
// been trusted with anything.
bool KnownDevices::isKnown(const QString &fingerprint) const
{
    return !fingerprint.isEmpty() && m_entries.value(fingerprint.toUpper()).paired;
}

QString KnownDevices::aliasFor(const QString &fingerprint) const
{
    return m_entries.value(fingerprint.toUpper()).alias;
}

QDateTime KnownDevices::firstSeen(const QString &fingerprint) const
{
    return m_entries.value(fingerprint.toUpper()).firstSeen;
}

QString KnownDevices::expectedFingerprint(const QString &alias) const
{
    if (alias.isEmpty())
        return QString();

    QHash<QString, Entry>::const_iterator it = m_entries.constBegin();
    for (; it != m_entries.constEnd(); ++it) {
        // Only pairings speak to what a name meant before. A blocked entry
        // carries the name the device used at the time it was blocked, and
        // letting that answer here would raise the impersonation warning on
        // the user's own device the moment it reused the name.
        if (it.value().paired && it.value().alias == alias)
            return it.key();
    }
    return QString();
}

bool KnownDevices::conflicts(const QString &fingerprint,
                             const QString &alias) const
{
    if (fingerprint.isEmpty() || alias.isEmpty())
        return false;

    // A key we already know is never a conflict, whatever it calls itself:
    // people rename their own devices, and the key is the identity.
    if (isKnown(fingerprint))
        return false;

    // A name we know arriving under a key we do not is the shape impersonation
    // takes here.
    const QString expected = expectedFingerprint(alias);
    return !expected.isEmpty() && !Crypto::equalsFold(expected, fingerprint);
}

void KnownDevices::remember(const QString &fingerprint, const QString &alias)
{
    if (fingerprint.isEmpty())
        return;

    const QString key = fingerprint.toUpper();
    QHash<QString, Entry>::iterator it = m_entries.find(key);

    if (it != m_entries.end()) {
        if (it.value().paired && it.value().alias == alias)
            return;   // nothing changed
        // The key is the identity, so a rename is the owner's business.
        it.value().alias = alias;
        it.value().paired = true;
    } else {
        Entry entry;
        entry.alias = alias;
        entry.firstSeen = QDateTime::currentDateTime();
        entry.paired = true;
        m_entries.insert(key, entry);
    }

    save();
    emit changed();
}

// Forgetting drops what we learned about a device. It deliberately does not
// drop a block: "forget this device" and "start accepting files from it
// again" are different intentions, and a button that quietly did the second
// while offering the first would be the wrong kind of surprise. Unblocking
// has its own page.
void KnownDevices::forget(const QString &fingerprint)
{
    const QString key = fingerprint.toUpper();
    QHash<QString, Entry>::iterator it = m_entries.find(key);
    if (it == m_entries.end())
        return;

    if (it.value().blocked) {
        // Keep the block, lose the pairing.
        it.value().alias.clear();
        it.value().paired = false;
        it.value().firstSeen = QDateTime::currentDateTime();
    } else {
        m_entries.erase(it);
    }

    save();
    emit changed();
}

void KnownDevices::forgetAll()
{
    if (m_entries.isEmpty())
        return;

    QHash<QString, Entry> kept;
    QHash<QString, Entry>::const_iterator it = m_entries.constBegin();
    for (; it != m_entries.constEnd(); ++it) {
        if (!it.value().blocked)
            continue;
        Entry entry;
        entry.blocked = true;
        entry.firstSeen = QDateTime::currentDateTime();
        kept.insert(it.key(), entry);
    }

    bool changedAnything = kept.count() != m_entries.count();
    if (!changedAnything) {
        QHash<QString, Entry>::const_iterator paired = m_entries.constBegin();
        for (; paired != m_entries.constEnd() && !changedAnything; ++paired)
            changedAnything = paired.value().paired;
    }
    if (!changedAnything)
        return;   // only blocks were on record; nothing to forget

    m_entries = kept;
    save();
    emit changed();
}

int KnownDevices::count() const
{
    int total = 0;
    QHash<QString, Entry>::const_iterator it = m_entries.constBegin();
    for (; it != m_entries.constEnd(); ++it) {
        if (it.value().paired)
            ++total;
    }
    return total;
}
