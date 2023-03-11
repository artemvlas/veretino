#include "treemodel.h"
#include "QDir"

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new TreeItem({tr("Path"), tr("Info")});
}

TreeModel::~TreeModel()
{
    delete rootItem;
}

void TreeModel::populateMap(const QMap<QString,QString> &map)
{
    QStringList filelist = map.keys();
    for (int f = 0; f < filelist.size(); ++f) {
        QStringList splitPath = filelist.at(f).split('/');

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
                QString info;
                if (var + 1 == splitPath.size())
                    info = map.value(filelist.at(f));
                TreeItem *ti = new TreeItem({splitPath.at(var), info}, parentItem);
                parentItem->appendChild(ti);
                parentItem = ti;
            }
        }
    }
}

QString TreeModel::getPath(const QModelIndex &index)
{
    QModelIndex newIndex = index;
    QString path = newIndex.data().toString();

    while (newIndex.parent().isValid()) {
        path = newIndex.parent().data().toString() + '/' + path;
        newIndex = newIndex.parent();
    }

    return path;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
    return rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}
