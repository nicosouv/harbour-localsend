#include "hashingfile.h"

HashingFile::HashingFile(const QString &name, QObject *parent)
    : QFile(name, parent)
    , m_hash(QCryptographicHash::Sha256)
{
}

qint64 HashingFile::writeData(const char *data, qint64 length)
{
    const qint64 written = QFile::writeData(data, length);

    // Only what actually reached the file is hashed. A short write means the
    // rest never arrived, and hashing it anyway would produce a digest for
    // something that is not on disk.
    if (written > 0)
        m_hash.addData(data, int(written));

    return written;
}

QString HashingFile::digest()
{
    return QString::fromLatin1(m_hash.result().toHex());
}
