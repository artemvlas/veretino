/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "treeitem.h"

TreeItem::TreeItem(const QVector<TreeItem::Data> &data, TreeItem *parent)
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

QVariant TreeItem::data(int column, quint8 role) const
{
    return (column < itemData.size() && column >= 0) ? itemData.at(column).data(role) : QVariant();
}

TreeItem *TreeItem::parent()
{
    return parentItem;
}

bool TreeItem::setData(int column, const QVariant &value, quint8 role)
{
    if (column < 0 || column >= itemData.size())
        return false;

    itemData[column].setData(value, role);
    return true;
}

void TreeItem::appendChild(TreeItem *item)
{
    childItems.append(item);
}

TreeItem *TreeItem::addChild(const QVector<TreeItem::Data> &tiData)
{
    TreeItem *ti = new TreeItem(tiData, this);
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
