#include "knowndevices.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

#include "crypto.h"

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
    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
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
        root.insert(it.key(), value);
    }

    QSaveFile file(storagePath());
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
    file.commit();

    QFile::setPermissions(storagePath(), QFile::ReadOwner | QFile::WriteOwner);
}

bool KnownDevices::isKnown(const QString &fingerprint) const
{
    return !fingerprint.isEmpty() && m_entries.contains(fingerprint.toUpper());
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
        if (it.value().alias == alias)
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
        if (it.value().alias == alias)
            return;   // nothing changed
        // The key is the identity, so a rename is the owner's business.
        it.value().alias = alias;
    } else {
        Entry entry;
        entry.alias = alias;
        entry.firstSeen = QDateTime::currentDateTime();
        m_entries.insert(key, entry);
    }

    save();
    emit changed();
}

void KnownDevices::forget(const QString &fingerprint)
{
    if (m_entries.remove(fingerprint.toUpper()) == 0)
        return;
    save();
    emit changed();
}

void KnownDevices::forgetAll()
{
    if (m_entries.isEmpty())
        return;
    m_entries.clear();
    save();
    emit changed();
}

int KnownDevices::count() const
{
    return m_entries.count();
}
