/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "treemodel.h"
#include "treemodeliterator.h"
#include "tools.h"
#include "pathstr.h"
#include "iconprovider.h"
#include <QDebug>

const QVector<QVariant> TreeModel::s_rootItemData = {
    QStringLiteral(u"Name"),
    QStringLiteral(u"Size"),
    QStringLiteral(u"Status"),
    QStringLiteral(u"Checksum"),
    QStringLiteral(u"ReChecksum"),
    QStringLiteral(u"Hashing Time"),
    QStringLiteral(u"Speed")
};

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_rootItem = new TreeItem(s_rootItemData);
}

TreeModel::~TreeModel()
{
    delete m_rootItem;
}

bool TreeModel::isEmpty() const
{
    return (m_rootItem->childCount() == 0);
}

void TreeModel::populate(const FileList &filesData)
{
    FileList::const_iterator iter;
    for (iter = filesData.constBegin(); iter != filesData.constEnd(); ++iter) {
        add_file(iter.key(), iter.value());
    }
}

bool TreeModel::add_file_unforced(const QString &filePath, const FileValues &values)
{
    const TreeItem *parentItem = add_folder(pathstr::parentFolder(filePath));

    if (parentItem->findChild(pathstr::basicName(filePath))) {
        return false;
    }

    add_file(filePath, values);
    return true;
}

void TreeModel::add_file(const QString &filePath, const FileValues &values)
{
    // data preparation
    QVector<QVariant> tiData(m_rootItem->columnCount());
    tiData[ColumnName] = pathstr::basicName(filePath);

    if (values.size >= 0)
        tiData[ColumnSize] = values.size;

    tiData[ColumnStatus] = QVariant::fromValue(values.status);

    if (!values.checksum.isEmpty())
        tiData[ColumnChecksum] = values.checksum;

    // item adding
    TreeItem *parentItem = add_folder(pathstr::parentFolder(filePath));
    parentItem->addChild(tiData);
}

TreeItem *TreeModel::add_folder(const QString &path)
{
    if (path.isEmpty())
        return m_rootItem;

    if (m_cacheFolderItems.contains(path)) {
        return m_cacheFolderItems.value(path);
    }

    TreeItem *parentItem = m_rootItem;
    const QStringList pathParts = path.split('/', Qt::SkipEmptyParts);

    for (const QString &_subFolder : pathParts) {
        TreeItem *ti = parentItem->findChild(_subFolder);
        if (ti) {
            parentItem = ti;
        } else {
            QVector<QVariant> tiData(m_rootItem->columnCount());
            tiData[ColumnName] = _subFolder;
            parentItem = parentItem->addChild(tiData);
        }
    }

    m_cacheFolderItems.insert(path, parentItem);
    return parentItem;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    TreeItem *parentItem = getItem(parent);
    if (!parentItem)
        return QModelIndex();

    TreeItem *childItem = parentItem->child(row);

    return childItem ? createIndex(row, column, childItem)
                     : QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &curIndex) const
{
    if (!curIndex.isValid())
        return QModelIndex();

    TreeItem *childItem = getItem(curIndex);
    TreeItem *parentItem = childItem ? childItem->parent() : nullptr;

    if (parentItem == m_rootItem || !parentItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() > 0)
        return 0;

    const TreeItem *parentItem = getItem(parent);

    return parentItem ? parentItem->childCount() : 0;
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &curIndex, int role) const
{
    if (!curIndex.isValid())
        return QVariant();

    if (role == Qt::DecorationRole) {
        static IconProvider s_icons;
        if (curIndex.column() == ColumnName) {
            return isFileRow(curIndex) ? s_icons.icon(curIndex.data().toString())
                                       : s_icons.iconFolder();
        }
        if (curIndex.column() == ColumnStatus && isFileRow(curIndex)) {
            return s_icons.icon(curIndex.data(RawDataRole).value<FileStatus>());
        }
    }

    if (role == Qt::ForegroundRole) {
        if (curIndex.column() == ColumnStatus || curIndex.column() == ColumnChecksum) {
            switch (itemFileStatus(curIndex)) {
            case FileStatus::Matched:
                return QColor(Qt::darkGreen);
            case FileStatus::Mismatched:
                return QColor(Qt::red);
            case FileStatus::UnPermitted:
            case FileStatus::ReadError:
                return QColor(Qt::darkRed);
            default: break;
            }
        } else if (curIndex.column() == ColumnReChecksum) {
            return QColor(Qt::darkGreen);
        }
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole && role != RawDataRole)
        return QVariant();

    const QVariant tiData = getItem(curIndex)->data(curIndex.column());

    if (tiData.isValid() && role != RawDataRole) {
        switch(curIndex.column()) {
        case ColumnSize:
            return format::dataSizeReadable(tiData.toLongLong());
        case ColumnStatus:
            return format::fileItemStatus(tiData.value<FileStatus>());
        case ColumnHashTime:
            return format::msecsToReadable(tiData.toLongLong());
        default:
            break;
        }
    }

    if (curIndex.column() == ColumnSpeed) {
        const qint64 hash_time = itemHashTime(curIndex);
        if (hash_time >= 0)
            return format::processSpeed(hash_time, itemFileSize(curIndex));
    }

    return tiData;
}

bool TreeModel::setData(const QModelIndex &curIndex, const QVariant &value, int role)
{
    if (role != Qt::EditRole || !curIndex.isValid())
        return false;

    TreeItem *ti = getItem(curIndex);
    const bool result = ti->setData(curIndex.column(), value);

    if (result) { // to change the color of the checksum during the verification process
        const QModelIndex &endIndex = (curIndex.column() == ColumnStatus) ? curIndex.siblingAtColumn(ColumnChecksum)
                                                                          : curIndex;

        emit dataChanged(curIndex, endIndex, { Qt::DisplayRole, Qt::EditRole, RawDataRole });
    }

    return result;
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &curIndex) const
{
    if (!curIndex.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable | QAbstractItemModel::flags(curIndex);
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return m_rootItem->data(section);

    return QVariant();
}

TreeItem *TreeModel::getItem(const QModelIndex &curIndex) const
{
    if (curIndex.isValid()) {
        TreeItem *ti = static_cast<TreeItem*>(curIndex.internalPointer());
        if (ti)
            return ti;
    }
    return m_rootItem;
}

QString TreeModel::getPath(const QModelIndex &curIndex, const QModelIndex &root)
{
    QString path;
    QModelIndex ind = curIndex.siblingAtColumn(ColumnName);

    if (ind.isValid()) {
        path = ind.data().toString();

        while (ind = ind.parent(),
               ind.isValid() && ind != root)
        {
            path = pathstr::joinPath(ind.data().toString(), path);
        }
    }

    return path;
}

QModelIndex TreeModel::getIndex(const QString &path, const QAbstractItemModel *model)
{
    if (path.isEmpty())
        return QModelIndex();

    QModelIndex parentIndex;
    QModelIndex curIndex = model->index(0, 0);
    const QStringList subfolders = path.split('/', Qt::SkipEmptyParts);

    for (const QString &_subfolder : subfolders) {
        for (int i = 0; curIndex.isValid(); ++i) {
            curIndex = model->index(i, 0, parentIndex);
            if (curIndex.data().toString() == _subfolder) {
                parentIndex = curIndex;
                break;
            }
        }
    }

    return curIndex;
}

void TreeModel::clearCacheFolderItems()
{
    m_cacheFolderItems.clear();
}

// the TreeModel implies that if an item has children,
// then it is a folder (or invalid-root); if not, then it is a file
bool TreeModel::isFileRow(const QModelIndex &curIndex)
{
    const QModelIndex ind = curIndex.siblingAtColumn(ColumnName);

    return (ind.isValid() && !ind.model()->hasChildren(ind));
}

bool TreeModel::isFolderRow(const QModelIndex &curIndex)
{
    const QModelIndex ind = curIndex.siblingAtColumn(ColumnName);

    return (ind.isValid() && ind.model()->hasChildren(ind));
}

bool TreeModel::hasChecksum(const QModelIndex &fileIndex)
{
    return fileIndex.siblingAtColumn(ColumnChecksum).data().isValid();
}

bool TreeModel::hasReChecksum(const QModelIndex &fileIndex)
{
    return fileIndex.siblingAtColumn(ColumnReChecksum).data().isValid();
}

bool TreeModel::hasStatus(const FileStatuses flag, const QModelIndex &fileIndex)
{
    return (flag & itemFileStatus(fileIndex));
}

bool TreeModel::contains(const FileStatuses flag, const QModelIndex &folderIndex)
{
    if (isFolderRow(folderIndex)) {
        TreeModelIterator it(folderIndex.model(), folderIndex);
        while (it.hasNext()) {
            if (flag & it.nextFile().status()) {
                return true;
            }
        }
    }

    return false;
}

QString TreeModel::itemName(const QModelIndex &curIndex)
{
    return curIndex.siblingAtColumn(ColumnName).data().toString();
}

qint64 TreeModel::itemFileSize(const QModelIndex &fileIndex)
{
    return fileIndex.siblingAtColumn(ColumnSize).data(RawDataRole).toLongLong();
}

FileStatus TreeModel::itemFileStatus(const QModelIndex &fileIndex)
{
    return fileIndex.siblingAtColumn(ColumnStatus).data(RawDataRole).value<FileStatus>();
}

QString TreeModel::itemFileChecksum(const QModelIndex &fileIndex)
{
    return fileIndex.siblingAtColumn(ColumnChecksum).data().toString();
}

QString TreeModel::itemFileReChecksum(const QModelIndex &fileIndex)
{
    return fileIndex.siblingAtColumn(ColumnReChecksum).data().toString();
}

qint64 TreeModel::itemHashTime(const QModelIndex &fileIndex)
{
    const QVariant val = fileIndex.siblingAtColumn(ColumnHashTime).data(RawDataRole);

    return val.isValid() ? val.toLongLong() : -1;
}
