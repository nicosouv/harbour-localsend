#ifndef HASHINGFILE_H
#define HASHINGFILE_H

#include <QCryptographicHash>
#include <QFile>
#include <QString>

// A file that hashes what it is given on the way past.
//
// The alternative is reading the whole thing back once it has landed, which
// on a phone means touching every byte of a two-gigabyte video a second time
// for no reason: the bytes are already in memory on their way to the disk.
// This costs the hash and nothing else.
//
// What the digest is worth is worth stating plainly, because it is easy to
// overrate. The sha256 in prepare-upload travels the same channel as the file
// does, so anybody able to alter one can alter the other - it is a checksum,
// not a MAC, and it authenticates nothing. Under TLS the integrity guarantee
// already comes from the transport. What this actually catches is accidental
// corruption, which TCP's sixteen-bit checksum lets through more often than
// people expect, and it lets us answer 422 the way the protocol says to.
class HashingFile : public QFile
{
public:
    explicit HashingFile(const QString &name, QObject *parent = 0);

    // Lowercase hex of everything written so far. Reading it finalises the
    // hash, so call it once, after the last write.
    QString digest();

protected:
    qint64 writeData(const char *data, qint64 length);

private:
    QCryptographicHash m_hash;
};

#endif // HASHINGFILE_H
