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
                    if (iter.value().isNew)
                        avail = QString("New file: %1").arg(format::dataSizeReadable(iter.value().size));
                    else if (!iter.value().exists)
                        avail = "Missing file";
                    else if (!iter.value().isReadable)
                        avail = "Unreadable";
                    else
                        avail = format::dataSizeReadable(iter.value().size);

                    // Status column
                    switch (iter.value().status) {
                    case FileValues::Matched:
                        status = "✓ OK";
                        break;
                    case FileValues::Mismatched:
                        status = "☒ NOT match";
                        break;
                    case FileValues::ChecksumUpdated:
                        status = "↻ stored checksum updated"; // 🗘
                        break;
                    case FileValues::Added:
                        status = "→ added to DB"; // ➔
                        break;
                    case FileValues::Removed:
                        status = "✂ removed from DB";
                        break;
                    }
                }

                TreeItem *ti = new TreeItem({splitPath.at(var), avail, status}, parentItem);
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
