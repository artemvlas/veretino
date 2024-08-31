/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "treemodel.h"
#include "treemodeliterator.h"
#include "tools.h"
#include <QDebug>

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new TreeItem({ "Name", "Size", "Status", "Checksum", "ReChecksum" });
}

TreeModel::~TreeModel()
{
    delete rootItem;
    // qDebug() << Q_FUNC_INFO << this;
}

bool TreeModel::isEmpty() const
{
    return (rootItem->childCount() == 0);
}

/* // OLD, no cache; will be removed until the next release
bool TreeModel::addFile(const QString &filePath, const FileValues &values)
{
    bool isAdded = false;
    const QStringList &splitPath = filePath.split('/');
    TreeItem *parentItem = rootItem;

    for (int var = 0; var < splitPath.size(); ++var) {
        bool not_exist = true;

        for (int i = 0; i < parentItem->childCount(); ++i) {
            if (parentItem->child(i)->data(0) == splitPath.at(var)) {
                parentItem = parentItem->child(i);
                not_exist = false;
                break;
            }
        }

        if (not_exist) {
            QVector<QVariant> iData(rootItem->columnCount());
            iData[ColumnName] = splitPath.at(var);

            // the last item is considered a file
            if (var + 1 == splitPath.size()) {
                if (values.size > 0)
                    iData[ColumnSize] = values.size;

                iData[ColumnStatus] = QVariant::fromValue(values.status);

                if (!values.checksum.isEmpty())
                    iData[ColumnChecksum] = values.checksum;

                isAdded = true;
            }

            TreeItem *ti = new TreeItem(iData, parentItem);
            parentItem->appendChild(ti);
            parentItem = ti;
        }
    }

    return isAdded;
} */ // DEPRECATED ^^^

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
    const QStringList &pathParts = path.split('/');

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

    if (isColored && role == Qt::ForegroundRole) {
        if (curIndex.column() == ColumnStatus || curIndex.column() == ColumnChecksum) {
            switch (itemFileStatus(curIndex)) {
            case FileStatus::Matched:
                return QColor(Qt::darkGreen);
            case FileStatus::Mismatched:
                return QColor(Qt::red);
            case FileStatus::Unreadable:
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

    QVariant &&iData = getItem(curIndex)->data(curIndex.column());

    if (iData.isValid() && role != RawDataRole) {
        if (curIndex.column() == ColumnSize)
            return format::dataSizeReadable(iData.toLongLong());
        if (curIndex.column() == ColumnStatus)
            return format::fileItemStatus(iData.value<FileStatus>());
    }

    return iData;
}

bool TreeModel::setData(const QModelIndex &curIndex, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    TreeItem *item = getItem(curIndex);
    bool result = item->setData(curIndex.column(), value);

    if (result) { // to change the color of the checksum during the verification process
        const QModelIndex &endIndex = (isColored && (curIndex.column() == ColumnStatus)) ? curIndex.siblingAtColumn(ColumnChecksum)
                                                                                         : curIndex;

        emit dataChanged(curIndex, endIndex, {Qt::DisplayRole, Qt::EditRole, RawDataRole});
        // It was before:
        // emit dataChanged(curIndex, curIndex, {Qt::DisplayRole, Qt::EditRole, RawDataRole});
    }

    return result;
}

bool TreeModel::setRowData(const QModelIndex &curIndex, Column column, const QVariant &itemData)
{
    if (!curIndex.isValid()) {
        qDebug() << "TreeModel::setRowData | Invalid index";
        return false;
    }

    const QModelIndex &idx = curIndex.siblingAtColumn(column);

    return idx.isValid() && setData(idx, itemData);
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &curIndex) const
{
    if (!curIndex.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable | QAbstractItemModel::flags(curIndex);
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
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
    QModelIndex newIndex = curIndex.siblingAtColumn(ColumnName);

    if (newIndex.isValid()) {
        path = newIndex.data().toString();

        while (newIndex.parent().isValid() && newIndex.parent() != root) {
            path = paths::joinPath(newIndex.parent().data().toString(), path);
            newIndex = newIndex.parent();
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
    const QStringList &parts = path.split('/');

    foreach (const QString &str, parts) {
        for (int i = 0; curIndex.isValid(); ++i) {
            curIndex = model->index(i, 0, parentIndex);
            if (curIndex.data().toString() == str) {
                //qDebug() << "***" << str << "finded on" << i << "row";
                parentIndex = model->index(i, 0, parentIndex);
                break;
            }
            //qDebug() << "*** Looking for:" << str << curIndex.data();
        }
    }
    //qDebug() << "View::pathToIndex" << path << "-->" << curIndex << curIndex.data();

    return curIndex;
}

void TreeModel::clearCreationCache()
{
    qDebug() << "TreeModel::clearCreationCache >>" << cacheFolderItems_.size();
    cacheFolderItems_.clear();
}

// the TreeModel implies that if an item has children, then it is a folder (or invalid-root); if not, then it is a file
bool TreeModel::isFileRow(const QModelIndex &curIndex)
{
    const QModelIndex &ind = curIndex.siblingAtColumn(ColumnName);

    return (ind.isValid() && !ind.model()->hasChildren(ind));
}

bool TreeModel::isFolderRow(const QModelIndex &curIndex)
{
    const QModelIndex &ind = curIndex.siblingAtColumn(ColumnName);

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
    return flag & itemFileStatus(fileIndex);
}

bool TreeModel::contains(const FileStatuses flag, const QModelIndex &folderIndex)
{
    bool isAny = false;

    if (isFolderRow(folderIndex)) {
        TreeModelIterator it(folderIndex.model(), folderIndex);
        while (it.hasNext()) {
            if (flag & itemFileStatus(it.nextFile().index())) {
                isAny = true;
                break;
            }
        }
    }

    return isAny;
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

void TreeModel::setColoredItems(const bool colored)
{
    isColored = colored;
}
