#include "treemodel.h"
#include "tools.h"

TreeModel::TreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new TreeItem({"Path", "Size / Availability", "Status"});
}

TreeModel::~TreeModel()
{
    delete rootItem;
}

void TreeModel::populate(const FileList &filesData)
{
    FileList::const_iterator iter;
    for (iter = filesData.constBegin(); iter != filesData.constEnd(); ++iter) {
        QStringList splitPath = iter.key().split('/');
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
                QString avail, status;
                if (var + 1 == splitPath.size()) {
                    // Size/Avail. column
                    switch (iter.value().status) {
                    case FileValues::New:
                        avail = QString("New file: %1").arg(format::dataSizeReadable(iter.value().size));
                        break;
                    case FileValues::Missing:
                        avail = "Missing file";
                        break;
                    case FileValues::Unreadable:
                        avail = "Unreadable";
                        break;
                    default:
                        avail = format::dataSizeReadable(iter.value().size);
                    }

                    // Status column
                    status = format::fileItemStatus(iter.value().status);
                }

                TreeItem *ti = new TreeItem({splitPath.at(var), avail, status}, parentItem);
                parentItem->appendChild(ti);
                parentItem = ti;
            }
        }
    }
}

QString TreeModel::getPath(const QModelIndex &curIndex)
{
    QModelIndex newIndex = index(curIndex.row(), 0 , curIndex.parent());
    QString path = newIndex.data().toString();

    while (newIndex.parent().isValid()) {
        path = paths::joinPath(newIndex.parent().data().toString(), path);
        newIndex = newIndex.parent();
    }

    return path;
}

QModelIndex TreeModel::getIndex(const QString &path)
{
    QModelIndex curIndex = index(0, 0);

    if (!path.isEmpty()) {
        QStringList parts = path.split('/');
        QModelIndex parentIndex;

        foreach (const QString &str, parts) {
            for (int i = 0; curIndex.isValid(); ++i) {
                curIndex = index(i, 0, parentIndex);
                if (curIndex.data().toString() == str) {
                    //qDebug() << "***" << str << "finded on" << i << "row";
                    parentIndex = index(i, 0, parentIndex);
                    break;
                }
                //qDebug() << "*** Looking for:" << str << curIndex.data();
            }
        }
        //qDebug() << "View::pathToIndex" << path << "-->" << curIndex << curIndex.data();
    }

    return curIndex;
}

void TreeModel::setItemStatus(const QString &itemPath, int status)
{
    QModelIndex curIndex = getIndex(itemPath);
    if (curIndex.isValid()) {
        setData(index(curIndex.row(), 2, curIndex.parent()),
                format::fileItemStatus(status));
    }
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    TreeItem *parentItem = getItem(parent);
    if (!parentItem)
        return QModelIndex();

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = getItem(index);
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

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    TreeItem *item = getItem(index);

    return item->data(index.column());
}

bool TreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    TreeItem *item = getItem(index);
    bool result = item->setData(index.column(), value);

    if (result)
        emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});

    return result;
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

TreeItem *TreeModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}
