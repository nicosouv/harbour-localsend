#include "securestore.h"

#include <QDebug>
#include <QFile>
#include <QSaveFile>

#include "crypto.h"

#ifdef HAVE_SAILFISH_SECRETS
#include <Secrets/createcollectionrequest.h>
#include <Secrets/result.h>
#include <Secrets/secret.h>
#include <Secrets/secretmanager.h>
#include <Secrets/storedsecretrequest.h>
#include <Secrets/storesecretrequest.h>
#endif

namespace {

const char *CollectionName = "harbour-localsend";
const char *SecretName = "storage-key";

// Marks a file as encrypted. A file without it is plaintext from an older
// install, or from a run where no keystore was available, and is read as-is.
// Cheaper and more honest than guessing from the contents.
const char *Magic = "LSENC1";

// Set by the tests. Not a way to inject a key on a device: nothing writes it
// outside setMasterKeyForTesting().
QByteArray s_testKey;

} // namespace

SecureStore::SecureStore()
{
}

SecureStore &SecureStore::instance()
{
    static SecureStore store;
    return store;
}

void SecureStore::setMasterKeyForTesting(const QByteArray &key)
{
    s_testKey = key;
    // Drop whatever the instance is holding so the next open() takes the new
    // key, which is what a test switching between modes needs.
    instance().m_key.clear();
}

bool SecureStore::isEncrypting() const
{
    return m_key.size() == Crypto::KeyBytes;
}

QString SecureStore::lastError() const
{
    return m_lastError;
}

bool SecureStore::open()
{
    if (isEncrypting())
        return true;

    if (!s_testKey.isEmpty()) {
        m_key = s_testKey;
        return isEncrypting();
    }

    if (fetchKey())
        return true;

    // Deliberately not fatal. Refusing to run because a keystore is missing
    // would break the one thing the app is for, and the files are still
    // owner-only inside the sandbox on a LUKS home.
    qWarning("localsend: storing data unencrypted (%s)", qPrintable(m_lastError));
    return false;
}

#ifdef HAVE_SAILFISH_SECRETS

bool SecureStore::fetchKey()
{
    using namespace Sailfish::Secrets;

    SecretManager manager;

    const Secret::Identifier identifier(
        QLatin1String(SecretName), QLatin1String(CollectionName),
        SecretManager::DefaultStoragePluginName);

    // Created on first run; an "already exists" failure here is the normal
    // second-run answer and is not treated as an error, because the read
    // below is what actually decides.
    CreateCollectionRequest create;
    create.setManager(&manager);
    create.setCollectionName(QLatin1String(CollectionName));
    create.setStoragePluginName(SecretManager::DefaultStoragePluginName);
    create.setEncryptionPluginName(SecretManager::DefaultEncryptionPluginName);
    create.setCollectionLockType(CreateCollectionRequest::DeviceLock);
    // Unlocked for as long as the device is: the app must reach its key while
    // the screen is locked, or an incoming transfer at 3am cannot be written.
    create.setDeviceLockUnlockSemantic(SecretManager::DeviceLockKeepUnlocked);
    create.setAccessControlMode(SecretManager::OwnerOnlyMode);
    create.setUserInteractionMode(SecretManager::PreventInteraction);
    create.startRequest();
    create.waitForFinished();

    StoredSecretRequest read;
    read.setManager(&manager);
    read.setIdentifier(identifier);
    read.setUserInteractionMode(SecretManager::PreventInteraction);
    read.startRequest();
    read.waitForFinished();

    if (read.result().code() == Result::Succeeded
            && read.secret().data().size() == Crypto::KeyBytes) {
        m_key = read.secret().data();
        m_lastError.clear();
        return true;
    }

    const QByteArray fresh = Crypto::randomBytes(Crypto::KeyBytes);
    if (fresh.isEmpty()) {
        m_lastError = QStringLiteral("no secure random source");
        return false;
    }

    Secret secret(identifier);
    secret.setData(fresh);

    StoreSecretRequest store;
    store.setManager(&manager);
    store.setSecretStorageType(StoreSecretRequest::CollectionSecret);
    store.setSecret(secret);
    store.setUserInteractionMode(SecretManager::PreventInteraction);
    store.startRequest();
    store.waitForFinished();

    if (store.result().code() != Result::Succeeded) {
        m_lastError = store.result().errorMessage();
        return false;
    }

    m_key = fresh;
    m_lastError.clear();
    return true;
}

#else

bool SecureStore::fetchKey()
{
    // Built without the Sailfish Secrets client library, which is the case in
    // every lane that is not the device. There is nowhere to keep a key that
    // an attacker reading these files could not read as well, so nothing is
    // encrypted rather than pretending otherwise.
    m_lastError = QStringLiteral("no keystore in this build");
    return false;
}

#endif

QByteArray SecureStore::read(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();

    const QByteArray contents = file.readAll();
    const QByteArray magic(Magic);

    if (!contents.startsWith(magic)) {
        // Written before there was a key, or by a build without a keystore.
        // Returned as it stands: an install that gains a keystore migrates on
        // its next write rather than needing a separate upgrade path.
        return contents;
    }

    if (!isEncrypting())
        return QByteArray();   // encrypted, and we have no key for it

    return Crypto::decrypt(contents.mid(magic.size()), m_key);
}

bool SecureStore::write(const QString &path, const QByteArray &data) const
{
    QByteArray payload = data;
    if (isEncrypting()) {
        const QByteArray sealed = Crypto::encrypt(data, m_key);
        if (sealed.isEmpty())
            return false;
        payload = QByteArray(Magic) + sealed;
    }

    // Through a temporary: a crash mid-write would otherwise leave a file
    // that is neither the old contents nor the new.
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    if (file.write(payload) != payload.size()) {
        file.cancelWriting();
        return false;
    }
    if (!file.commit())
        return false;

    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
    return true;
}
