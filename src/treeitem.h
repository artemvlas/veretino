/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef TREEITEM_H
#define TREEITEM_H
#include <QVariant>

class TreeItem
{
public:
    explicit TreeItem(const QList<QVariant> &data, TreeItem *parent = nullptr);
    ~TreeItem();

    TreeItem *child(int number);
    TreeItem *parent();
    int childNumber() const;
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    bool setData(int column, const QVariant &value);
    void appendChild(TreeItem *child);

private:
    QList<TreeItem*> childItems;
    QList<QVariant> itemData;
    TreeItem *parentItem;
};

#endif // TREEITEM_H
