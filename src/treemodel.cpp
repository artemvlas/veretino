// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "treemodel.h"
#include "tools.h"
#include <QDebug>

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new TreeItem({"Path", "Size", "Status", "Checksum", "ReChecksum"}); // "Size / Availability"
}

TreeModel::~TreeModel()
{
    delete rootItem;
    qDebug() << this << "deleted";
}

bool TreeModel::isEmpty()
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
            QVariant placebo;
            QList<QVariant> iData {splitPath.at(var), placebo, placebo, placebo, placebo};

            // the last item is considered a file
            if (var + 1 == splitPath.size()) {
                if (values.size > 0)
                    iData.replace(ModelKit::ColumnSize, values.size);

                iData.replace(ModelKit::ColumnStatus, QVariant::fromValue(values.status));

                if (!values.checksum.isEmpty())
                    iData.replace(ModelKit::ColumnChecksum, values.checksum);

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

bool TreeModel::setItemData(const QModelIndex &curIndex, ModelKit::Columns column, const QVariant &itemData)
{   
    if (!curIndex.isValid() || curIndex.model() != this) {
        qDebug() << "TreeModel::setItemData | Wrong index";
        return false;
    }

    QModelIndex idx = ModelKit::siblingAtRow(curIndex, column);

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
    using namespace ModelKit;

    if (!curIndex.isValid())
        return QVariant();

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

    if (result)
        emit dataChanged(curIndex, curIndex, {Qt::DisplayRole, Qt::EditRole, ModelKit::RawDataRole});

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
