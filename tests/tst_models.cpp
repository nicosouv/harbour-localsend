#include <QDir>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include "devicemodel.h"
#include "historymodel.h"
#include "selectionmodel.h"
#include "transfermodel.h"

class TestModels : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void transferTracksBytesAcrossFiles();
    void finishedFileCountsForItsWholeSize();
    void transferProgressSurvivesZeroByteFiles();
    void transferNamesItsStateForQml();

    void devicesAreKeyedByFingerprint();
    void devicesStaySortedByAlias();
    void devicesAgeOut();

    void selectionRejectsDuplicatesAndMissingFiles();

    void historyKeepsNewestFirst();

private:
    QList<FileEntry> threeFiles() const;
};

void TestModels::initTestCase()
{
    // HistoryModel writes to AppDataLocation; test mode keeps that out of the
    // developer's real profile.
    QStandardPaths::setTestModeEnabled(true);
}

QList<FileEntry> TestModels::threeFiles() const
{
    QList<FileEntry> files;
    for (int i = 0; i < 3; ++i) {
        FileEntry entry;
        entry.id = QStringLiteral("id%1").arg(i);
        entry.fileName = QStringLiteral("file%1.bin").arg(i);
        entry.size = 100;
        files.append(entry);
    }
    return files;
}

void TestModels::transferTracksBytesAcrossFiles()
{
    TransferModel transfer;
    transfer.begin(TransferModel::Sending, QStringLiteral("Peer"),
                   QStringLiteral("10.0.0.2"), QStringLiteral("desktop"),
                   threeFiles());

    QCOMPARE(transfer.fileCount(), 3);
    QCOMPARE(transfer.totalBytes(), qint64(300));
    QCOMPARE(transfer.transferredBytes(), qint64(0));

    transfer.setFileTransferred(0, 50);
    QCOMPARE(transfer.transferredBytes(), qint64(50));

    // Absolute, not cumulative: a second report of the same count must not
    // double-count, which is the bug that makes progress bars overshoot.
    transfer.setFileTransferred(0, 50);
    QCOMPARE(transfer.transferredBytes(), qint64(50));

    transfer.setFileTransferred(0, 100);
    transfer.setFileTransferred(1, 25);
    QCOMPARE(transfer.transferredBytes(), qint64(125));
    QCOMPARE(transfer.progress(), 125.0 / 300.0);

    // A count past the file's own size is a lie the model refuses to record.
    transfer.setFileTransferred(1, 999);
    QCOMPARE(transfer.transferredBytes(), qint64(200));

    QCOMPARE(transfer.indexOfFile(QStringLiteral("id2")), 2);
    QCOMPARE(transfer.indexOfFile(QStringLiteral("nope")), -1);
}

void TestModels::finishedFileCountsForItsWholeSize()
{
    TransferModel transfer;
    transfer.begin(TransferModel::Receiving, QStringLiteral("Peer"),
                   QStringLiteral("10.0.0.2"), QStringLiteral("mobile"),
                   threeFiles());

    // The last progress report before completion routinely goes missing, and
    // a transfer that ends at 99% looks broken however correct it was.
    transfer.setFileTransferred(0, 90);
    transfer.setFileStatus(0, TransferModel::FileDone);

    QCOMPARE(transfer.transferredBytes(), qint64(100));
    QCOMPARE(transfer.completedCount(), 1);
    QCOMPARE(transfer.currentIndex(), 1);

    transfer.setFileStatus(1, TransferModel::FileFailed, QStringLiteral("nope"));
    transfer.setFileStatus(2, TransferModel::FileSkipped);
    QCOMPARE(transfer.failedCount(), 2);
    QCOMPARE(transfer.completedCount(), 1);
}

void TestModels::transferProgressSurvivesZeroByteFiles()
{
    QList<FileEntry> files;
    for (int i = 0; i < 2; ++i) {
        FileEntry entry;
        entry.id = QStringLiteral("z%1").arg(i);
        entry.fileName = QStringLiteral("empty%1").arg(i);
        entry.size = 0;
        files.append(entry);
    }

    TransferModel transfer;
    transfer.begin(TransferModel::Sending, QStringLiteral("Peer"),
                   QStringLiteral("10.0.0.2"), QStringLiteral("desktop"), files);

    // Nothing to divide by, so progress falls back to counting files rather
    // than producing a NaN that paints an empty ring forever.
    QCOMPARE(transfer.totalBytes(), qint64(0));
    QCOMPARE(transfer.progress(), 0.0);

    transfer.setFileStatus(0, TransferModel::FileDone);
    QCOMPARE(transfer.progress(), 0.5);

    transfer.setFileStatus(1, TransferModel::FileDone);
    QCOMPARE(transfer.progress(), 1.0);
}

void TestModels::transferNamesItsStateForQml()
{
    TransferModel transfer;
    QCOMPARE(transfer.stateName(), QStringLiteral("idle"));
    QVERIFY(!transfer.active());

    transfer.begin(TransferModel::Sending, QStringLiteral("Peer"),
                   QStringLiteral("10.0.0.2"), QStringLiteral("desktop"),
                   threeFiles());

    transfer.setState(TransferModel::Requesting);
    QCOMPARE(transfer.stateName(), QStringLiteral("requesting"));
    QCOMPARE(transfer.directionName(), QStringLiteral("send"));
    QVERIFY(transfer.active());
    QVERIFY(transfer.sending());

    transfer.setState(TransferModel::Active);
    QCOMPARE(transfer.stateName(), QStringLiteral("active"));

    transfer.setState(TransferModel::Finished);
    QCOMPARE(transfer.stateName(), QStringLiteral("finished"));
    QVERIFY(!transfer.active());
}

void TestModels::devicesAreKeyedByFingerprint()
{
    DeviceModel devices;

    DeviceInfo phone;
    phone.fingerprint = QStringLiteral("aaa");
    phone.alias = QStringLiteral("Nice Orange");
    phone.address = QStringLiteral("192.168.1.10");

    QVERIFY(devices.upsert(phone));
    QCOMPARE(devices.count(), 1);

    // Same device, new address: an entry that moved, not a second device.
    phone.address = QStringLiteral("192.168.1.11");
    QVERIFY(!devices.upsert(phone));
    QCOMPARE(devices.count(), 1);
    QCOMPARE(devices.at(0).address, QStringLiteral("192.168.1.11"));

    // A device with no address cannot be reached, so it does not belong here.
    DeviceInfo unreachable;
    unreachable.fingerprint = QStringLiteral("bbb");
    QVERIFY(!devices.upsert(unreachable));
    QCOMPARE(devices.count(), 1);

    const QVariantMap row = devices.get(0);
    QCOMPARE(row.value(QStringLiteral("alias")).toString(),
             QStringLiteral("Nice Orange"));
    QCOMPARE(row.value(QStringLiteral("address")).toString(),
             QStringLiteral("192.168.1.11"));
    QVERIFY(devices.get(99).isEmpty());
}

void TestModels::devicesStaySortedByAlias()
{
    DeviceModel devices;

    const char *aliases[] = { "Zulu", "Alpha", "Mike" };
    for (int i = 0; i < 3; ++i) {
        DeviceInfo device;
        device.fingerprint = QStringLiteral("f%1").arg(i);
        device.alias = QLatin1String(aliases[i]);
        device.address = QStringLiteral("10.0.0.%1").arg(i + 1);
        devices.upsert(device);
    }

    QCOMPARE(devices.at(0).alias, QStringLiteral("Alpha"));
    QCOMPARE(devices.at(1).alias, QStringLiteral("Mike"));
    QCOMPARE(devices.at(2).alias, QStringLiteral("Zulu"));

    // A rename has to move the row, not just repaint it, or the list stops
    // being sorted and rows start jumping under people's fingers.
    DeviceInfo renamed = devices.byFingerprint(QStringLiteral("f0"));
    renamed.alias = QStringLiteral("Bravo");
    devices.upsert(renamed);

    QCOMPARE(devices.at(0).alias, QStringLiteral("Alpha"));
    QCOMPARE(devices.at(1).alias, QStringLiteral("Bravo"));
    QCOMPARE(devices.at(2).alias, QStringLiteral("Mike"));
    QCOMPARE(devices.count(), 3);
}

void TestModels::devicesAgeOut()
{
    DeviceModel devices;

    DeviceInfo fresh;
    fresh.fingerprint = QStringLiteral("new");
    fresh.alias = QStringLiteral("Fresh");
    fresh.address = QStringLiteral("10.0.0.1");
    devices.upsert(fresh);

    QCOMPARE(devices.count(), 1);

    // Nothing has had time to go stale.
    devices.prune(60);
    QCOMPARE(devices.count(), 1);

    // Anything older than zero seconds is everything.
    devices.prune(-1);
    QCOMPARE(devices.count(), 0);
}

void TestModels::selectionRejectsDuplicatesAndMissingFiles()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    const QString path = directory.path() + QStringLiteral("/photo.jpg");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray(1234, 'a'));
    file.close();

    SelectionModel selection;
    QVERIFY(selection.isEmpty());

    QVERIFY(selection.add(path));
    QCOMPARE(selection.count(), 1);
    QCOMPARE(selection.totalBytes(), qint64(1234));

    // The pickers hand back the same file twice more often than you would
    // think, and both spellings of it.
    QVERIFY(!selection.add(path));
    QVERIFY(!selection.add(QStringLiteral("file://") + path));
    QCOMPARE(selection.count(), 1);

    QVERIFY(!selection.add(directory.path() + QStringLiteral("/ghost.txt")));
    QVERIFY(!selection.add(directory.path()));   // a folder is not a file
    QCOMPARE(selection.count(), 1);

    QCOMPARE(selection.paths().first(), QFileInfo(path).canonicalFilePath());

    selection.removeAt(0);
    QVERIFY(selection.isEmpty());
    QCOMPARE(selection.totalBytes(), qint64(0));
}

void TestModels::historyKeepsNewestFirst()
{
    HistoryModel history;
    history.clear();

    for (int i = 0; i < 3; ++i) {
        HistoryModel::Record record;
        record.id = QStringLiteral("r%1").arg(i);
        record.direction = QStringLiteral("receive");
        record.peerAlias = QStringLiteral("Peer %1").arg(i);
        record.timestamp = QDateTime::currentDateTime();
        record.fileNames = QStringList() << QStringLiteral("f%1.bin").arg(i);
        record.totalBytes = 100 * (i + 1);
        record.status = QStringLiteral("finished");
        history.append(record);
    }

    QCOMPARE(history.count(), 3);
    QCOMPARE(history.data(history.index(0, 0), HistoryModel::RecordIdRole).toString(),
             QStringLiteral("r2"));
    QCOMPARE(history.data(history.index(0, 0), HistoryModel::FileCountRole).toInt(), 1);

    history.removeAt(0);
    QCOMPARE(history.count(), 2);
    QCOMPARE(history.data(history.index(0, 0), HistoryModel::RecordIdRole).toString(),
             QStringLiteral("r1"));

    // A fresh model reads what the previous one wrote.
    HistoryModel reloaded;
    QCOMPARE(reloaded.count(), 2);
    QCOMPARE(reloaded.data(reloaded.index(0, 0), HistoryModel::PeerAliasRole).toString(),
             QStringLiteral("Peer 1"));

    history.clear();
    QCOMPARE(history.count(), 0);
}

QTEST_MAIN(TestModels)
#include "tst_models.moc"
