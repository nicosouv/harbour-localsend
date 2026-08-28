#ifndef SECURESTORE_H
#define SECURESTORE_H

#include <QByteArray>
#include <QString>

// Reads and writes the app's own files, encrypted when the platform will hold
// a key for us.
//
// The honest limit first, because it decides what this is worth. This app has
// to read its own keys with nobody present - files arrive while the phone is
// locked - so whatever unwraps them must be reachable unattended. Anything
// reachable unattended is reachable by an attacker who can already run code as
// this user. Encrypting with a key sitting in the next file along would be
// theatre, and is not what happens here.
//
// What is worth something is a key this process never stores itself.
// sailfishsecretsd keeps one on our behalf, in its own store, released to this
// application and no other. That does not stop somebody running code as the
// user, but it does mean a *copy* of our data directory - a backup, a pulled
// image, an unpacked archive - decrypts to nothing. That is the threat this
// addresses, and the only one.
//
// Underneath the device lock, /home is LUKS anyway, so a powered-off handset
// was never readable to begin with. This is about the copies that leave it.
//
// When the daemon is unavailable the files are written in the clear rather
// than not at all: a transfer app that refuses to start because a keystore is
// missing has failed at the thing it exists for. isEncrypting() says which
// mode is in force, and the About page reports it.
class SecureStore
{
public:
    // One key per process, because there is one key: the daemon hands this
    // application a single storage key and every file shares it. Threading an
    // instance through five constructors would suggest a choice that does not
    // exist.
    static SecureStore &instance();

    // Fetches the master key, creating one on first run. Safe to call more
    // than once. False means we will be writing in the clear, with the reason
    // in lastError().
    bool open();

    bool isEncrypting() const;
    QString lastError() const;

    // Both transparently handle the other mode: read() accepts a plaintext
    // file written before a key existed, which is what migrates an install
    // forward without a separate upgrade step.
    QByteArray read(const QString &path) const;
    bool write(const QString &path, const QByteArray &data) const;

    // Replaces the process-wide key. Tests use it to exercise both modes
    // without a daemon; nothing else should.
    static void setMasterKeyForTesting(const QByteArray &key);

private:
    SecureStore();

    bool fetchKey();

    QByteArray m_key;
    QString m_lastError;
};

#endif // SECURESTORE_H
