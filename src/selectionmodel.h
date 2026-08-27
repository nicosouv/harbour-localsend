#ifndef SELECTIONMODEL_H
#define SELECTIONMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <QStringList>

// The files staged for the next send.
//
// It exists so the two orders work equally well: pick a device and then the
// files, or gather files first and choose a device afterwards. The tray at the
// bottom of the main page is this model, and it survives page changes because
// nothing else in the app should have to remember a half-made decision.
class SelectionModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(qint64 totalBytes READ totalBytes NOTIFY changed)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY changed)

public:
    struct Item
    {
        QString path;
        QString fileName;
        QString fileType;
        qint64 size;

        Item();
    };

    enum Roles {
        PathRole = Qt::UserRole + 1,
        FileNameRole,
        FileTypeRole,
        FileSizeRole
    };

    explicit SelectionModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

    int count() const;
    bool isEmpty() const;
    qint64 totalBytes() const;

    // Accepts plain paths and file:// URLs, which is what the Silica pickers
    // hand back depending on which one was used. Duplicates are ignored: the
    // pickers happily return the same file twice.
    Q_INVOKABLE bool add(const QString &pathOrUrl);
    Q_INVOKABLE int addAll(const QStringList &pathsOrUrls);
    Q_INVOKABLE void removeAt(int row);
    Q_INVOKABLE void clear();

    Q_INVOKABLE QStringList paths() const;

signals:
    void changed();

private:
    QList<Item> m_items;
};

#endif // SELECTIONMODEL_H
