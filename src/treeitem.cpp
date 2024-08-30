/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "treeitem.h"

TreeItem::TreeItem(const QVector<QVariant> &data, TreeItem *parent)
    : itemData(data), parentItem(parent)
{}

TreeItem::~TreeItem()
{
    qDeleteAll(childItems);
}

TreeItem *TreeItem::child(int number)
{
    return (number < childItems.size() && number >= 0) ? childItems.at(number) : nullptr;
}

int TreeItem::childCount() const
{
    return childItems.count();
}

int TreeItem::childNumber() const
{
    return parentItem ? parentItem->childItems.indexOf(const_cast<TreeItem*>(this)) : 0;
}

int TreeItem::columnCount() const
{
    return itemData.count();
}

QVariant TreeItem::data(int column) const
{
    return (column < itemData.size() && column >= 0) ? itemData.at(column) : QVariant();
}

TreeItem *TreeItem::parent()
{
    return parentItem;
}

bool TreeItem::setData(int column, const QVariant &value)
{
    if (column < 0 || column >= itemData.size())
        return false;

    itemData[column] = value;
    return true;
}

void TreeItem::appendChild(TreeItem *item)
{
    childItems.append(item);
}

TreeItem *TreeItem::addChild(const QVector<QVariant> &rowData)
{
    TreeItem *ti = new TreeItem(rowData, this);
    childItems.append(ti);
    return ti;
}

TreeItem *TreeItem::findChild(const QString &str) const
{
    for (TreeItem *chItem : childItems) {
        if (str == chItem->data(0).toString()) {
            return chItem;
        }
    }

    return nullptr;
}
