#include "selectionmodel.h"

#include <QFileInfo>
#include <QMimeDatabase>
#include <QUrl>

SelectionModel::Item::Item()
    : size(0)
{
}

SelectionModel::SelectionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SelectionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.count();
}

QVariant SelectionModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_items.count())
        return QVariant();

    const Item &item = m_items.at(index.row());
    switch (role) {
    case PathRole:     return item.path;
    case FileNameRole: return item.fileName;
    case FileTypeRole: return item.fileType;
    case FileSizeRole: return item.size;
    default:           return QVariant();
    }
}

QHash<int, QByteArray> SelectionModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[PathRole] = "path";
    roles[FileNameRole] = "fileName";
    roles[FileTypeRole] = "fileType";
    roles[FileSizeRole] = "fileSize";
    return roles;
}

int SelectionModel::count() const
{
    return m_items.count();
}

bool SelectionModel::isEmpty() const
{
    return m_items.isEmpty();
}

qint64 SelectionModel::totalBytes() const
{
    qint64 total = 0;
    for (int i = 0; i < m_items.count(); ++i)
        total += m_items.at(i).size;
    return total;
}

bool SelectionModel::add(const QString &pathOrUrl)
{
    QString path = pathOrUrl;
    if (path.startsWith(QLatin1String("file://")))
        path = QUrl(path).toLocalFile();
    if (path.isEmpty())
        return false;

    const QFileInfo info(path);
    if (!info.exists() || !info.isFile() || !info.isReadable())
        return false;

    const QString canonical = info.canonicalFilePath();
    for (int i = 0; i < m_items.count(); ++i) {
        if (m_items.at(i).path == canonical)
            return false;
    }

    Item item;
    item.path = canonical;
    item.fileName = info.fileName();
    item.size = info.size();

    QMimeDatabase mimeDatabase;
    item.fileType = mimeDatabase.mimeTypeForFile(
        info, QMimeDatabase::MatchExtension).name();

    beginInsertRows(QModelIndex(), m_items.count(), m_items.count());
    m_items.append(item);
    endInsertRows();

    emit changed();
    return true;
}

int SelectionModel::addAll(const QStringList &pathsOrUrls)
{
    int added = 0;
    for (int i = 0; i < pathsOrUrls.count(); ++i) {
        if (add(pathsOrUrls.at(i)))
            ++added;
    }
    return added;
}

void SelectionModel::removeAt(int row)
{
    if (row < 0 || row >= m_items.count())
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_items.removeAt(row);
    endRemoveRows();

    emit changed();
}

void SelectionModel::clear()
{
    if (m_items.isEmpty())
        return;

    beginResetModel();
    m_items.clear();
    endResetModel();

    emit changed();
}

QStringList SelectionModel::paths() const
{
    QStringList result;
    for (int i = 0; i < m_items.count(); ++i)
        result.append(m_items.at(i).path);
    return result;
}
