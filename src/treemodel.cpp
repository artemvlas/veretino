/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "treemodel.h"
#include "treemodeliterator.h"
#include "tools.h"
#include <QDebug>

const QVector<QVariant> TreeModel::s_rootItemData = {
    QStringLiteral(u"Name"),
    QStringLiteral(u"Size"),
    QStringLiteral(u"Status"),
    QStringLiteral(u"Checksum"),
    QStringLiteral(u"ReChecksum")
};

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new TreeItem(s_rootItemData);
}

TreeModel::~TreeModel()
{
    delete rootItem;
}

bool TreeModel::isEmpty() const
{
    return (rootItem->childCount() == 0);
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
    const TreeItem *parentItem = add_folder(paths::parentFolder(filePath));

    if (parentItem->findChild(paths::basicName(filePath))) {
        return false;
    }

    add_file(filePath, values);
    return true;
}

void TreeModel::add_file(const QString &filePath, const FileValues &values)
{
    // data preparation
    QVector<QVariant> _tiData(rootItem->columnCount());
    _tiData[ColumnName] = paths::basicName(filePath);

    if (values.size >= 0)
        _tiData[ColumnSize] = values.size;

    _tiData[ColumnStatus] = QVariant::fromValue(values.status);

    if (!values.checksum.isEmpty())
        _tiData[ColumnChecksum] = values.checksum;

    // item adding
    TreeItem *parentItem = add_folder(paths::parentFolder(filePath));
    parentItem->addChild(_tiData);
}

TreeItem *TreeModel::add_folder(const QString &path)
{
    if (path.isEmpty())
        return rootItem;

    if (cacheFolderItems_.contains(path)) {
        //qDebug() << "add_folder >> returned cached value:" << path;
        return cacheFolderItems_.value(path);
    }

    TreeItem *parentItem = rootItem;
    const QStringList pathParts = path.split('/', Qt::SkipEmptyParts);

    for (const QString &_subFolder : pathParts) {
        TreeItem *_ti = parentItem->findChild(_subFolder);
        if (_ti) {
            parentItem = _ti;
        }
        else {
            QVector<QVariant> _tiData(rootItem->columnCount());
            _tiData[ColumnName] = _subFolder;
            parentItem = parentItem->addChild(_tiData);
        }
    }

    cacheFolderItems_.insert(path, parentItem);
    //qDebug() << "add_folder >> value cached:" << path;
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

    if (parentItem == rootItem || !parentItem)
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
    return rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &curIndex, int role) const
{
    if (!curIndex.isValid())
        return QVariant();

    if (role == Qt::DecorationRole) {
        if (curIndex.column() == ColumnName) {
            return isFileRow(curIndex) ? icons_.icon(curIndex.data().toString())
                                       : icons_.iconFolder();
        }
        if (curIndex.column() == ColumnStatus && isFileRow(curIndex)) {
            return icons_.icon(curIndex.data(RawDataRole).value<FileStatus>());
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
        }
        else if (curIndex.column() == ColumnReChecksum) {
            return QColor(Qt::darkGreen);
        }
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole && role != RawDataRole)
        return QVariant();

    const QVariant _tiData = getItem(curIndex)->data(curIndex.column());

    if (_tiData.isValid() && role != RawDataRole) {
        if (curIndex.column() == ColumnSize)
            return format::dataSizeReadable(_tiData.toLongLong());
        if (curIndex.column() == ColumnStatus)
            return format::fileItemStatus(_tiData.value<FileStatus>());
    }

    return _tiData;
}

bool TreeModel::setData(const QModelIndex &curIndex, const QVariant &value, int role)
{
    if (role != Qt::EditRole || !curIndex.isValid())
        return false;

    TreeItem *item = getItem(curIndex);
    const bool result = item->setData(curIndex.column(), value);

    if (result) { // to change the color of the checksum during the verification process
        const QModelIndex &endIndex = (curIndex.column() == ColumnStatus) ? curIndex.siblingAtColumn(ColumnChecksum)
                                                                          : curIndex;

        emit dataChanged(curIndex, endIndex, { Qt::DisplayRole, Qt::EditRole, RawDataRole });
    }

    return result;
}
/*
bool TreeModel::setRowData(const QModelIndex &curIndex, Column column, const QVariant &value)
{
    return setData(curIndex.siblingAtColumn(column), value);
}*/

Qt::ItemFlags TreeModel::flags(const QModelIndex &curIndex) const
{
    if (!curIndex.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable | QAbstractItemModel::flags(curIndex);
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

TreeItem *TreeModel::getItem(const QModelIndex &curIndex) const
{
    if (curIndex.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(curIndex.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}

QString TreeModel::getPath(const QModelIndex &curIndex, const QModelIndex &root)
{
    QString path;
    QModelIndex _ind = curIndex.siblingAtColumn(ColumnName);

    if (_ind.isValid()) {
        path = _ind.data().toString();

        while (_ind = _ind.parent(),
               _ind.isValid() && _ind != root)
        {
            path = paths::joinPath(_ind.data().toString(), path);
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
    qDebug() << "TreeModel::clearCacheFolderItems >>" << cacheFolderItems_.size();
    cacheFolderItems_.clear();
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
