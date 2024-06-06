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
    // qDebug() << this << "deleted";
}

bool TreeModel::isEmpty() const
{
    return (rootItem->childCount() == 0);
}

bool TreeModel::addFile(const QString &filePath, const FileValues &values)
{
    bool isAdded = false;
    QStringList splitPath = filePath.split('/');
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
            TreeItem *ti;
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

            ti = new TreeItem(iData, parentItem);
            parentItem->appendChild(ti);
            parentItem = ti;
        }
    }

    return isAdded;
}

void TreeModel::populate(const FileList &filesData)
{
    FileList::const_iterator iter;
    for (iter = filesData.constBegin(); iter != filesData.constEnd(); ++iter) {
        addFile(iter.key(), iter.value());
    }
}

bool TreeModel::setRowData(const QModelIndex &curIndex, Column column, const QVariant &itemData)
{   
    if (!curIndex.isValid() || curIndex.model() != this) {
        qDebug() << "TreeModel::setRowData | Wrong index";
        return false;
    }

    QModelIndex idx = siblingAtRow(curIndex, column);

    return idx.isValid() && setData(idx, itemData);
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
            FileStatus status = itemFileStatus(curIndex);
            if (status == FileStatus::Matched)
                return QColor(Qt::darkGreen);
            else if (status == FileStatus::Mismatched)
                return QColor(Qt::red);
        }
        else if (curIndex.column() == ColumnReChecksum)
            return QColor(Qt::darkGreen);
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole && role != RawDataRole)
        return QVariant();

    QVariant iData = getItem(curIndex)->data(curIndex.column());

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
        const QModelIndex &endIndex = (isColored && (curIndex.column() == ColumnStatus)) ? siblingAtRow(curIndex, ColumnChecksum)
                                                                                         : curIndex;

        emit dataChanged(curIndex, endIndex, {Qt::DisplayRole, Qt::EditRole, RawDataRole});
        // It was before:
        // emit dataChanged(curIndex, curIndex, {Qt::DisplayRole, Qt::EditRole, RawDataRole});
    }

    return result;
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
    QModelIndex newIndex = siblingAtRow(curIndex, ColumnName);

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
    QModelIndex curIndex;

    if (!path.isEmpty()) {
        QModelIndex parentIndex;
        curIndex = model->index(0, 0);
        QStringList parts = path.split('/');

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
    }

    return curIndex;
}

QModelIndex TreeModel::siblingAtRow(const QModelIndex &curIndex, Column column)
{
    return curIndex.isValid() ? curIndex.model()->index(curIndex.row(), column, curIndex.parent())
                              : QModelIndex();
}

// the TreeModel implies that if an item has children, then it is a folder (or invalid-root); if not, then it is a file
bool TreeModel::isFileRow(const QModelIndex &curIndex)
{
    QModelIndex index = siblingAtRow(curIndex, ColumnName);

    return (index.isValid() && !index.model()->hasChildren(index));
}

bool TreeModel::isFolderRow(const QModelIndex &curIndex)
{
    QModelIndex index = siblingAtRow(curIndex, ColumnName);

    return (index.isValid() && index.model()->hasChildren(index));
}

bool TreeModel::hasChecksum(const QModelIndex &fileIndex)
{
    return siblingAtRow(fileIndex, ColumnChecksum).data().isValid();
}

bool TreeModel::hasReChecksum(const QModelIndex &fileIndex)
{
    return siblingAtRow(fileIndex, ColumnReChecksum).data().isValid();
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
    return siblingAtRow(curIndex, ColumnName).data().toString();
}

qint64 TreeModel::itemFileSize(const QModelIndex &fileIndex)
{
    return siblingAtRow(fileIndex, ColumnSize).data(RawDataRole).toLongLong();
}

FileStatus TreeModel::itemFileStatus(const QModelIndex &fileIndex)
{
    return siblingAtRow(fileIndex, ColumnStatus).data(RawDataRole).value<FileStatus>();
}

QString TreeModel::itemFileChecksum(const QModelIndex &fileIndex)
{
    return siblingAtRow(fileIndex, ColumnChecksum).data().toString();
}

QString TreeModel::itemFileReChecksum(const QModelIndex &fileIndex)
{
    return siblingAtRow(fileIndex, ColumnReChecksum).data().toString();
}

void TreeModel::setColoredItems(const bool colored)
{
    isColored = colored;
}
