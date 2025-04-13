/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef TREEITEM_H
#define TREEITEM_H
#include <QVariant>

class TreeItem
{
public:
    explicit TreeItem(const QVector<QVariant> &data, TreeItem *parent = nullptr);
    ~TreeItem();

    TreeItem *child(int number);
    TreeItem *parent();
    int childNumber() const;
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    bool setData(int column, const QVariant &value);
    void appendChild(TreeItem *child);

    // creates and appends a new child item, returns a pointer to it
    TreeItem *addChild(const QVector<QVariant> &tiData);

    // looks for matches in column 0
    TreeItem *findChild(const QString &str) const;

private:
    QList<TreeItem*> childItems;
    QVector<QVariant> itemData;
    TreeItem *parentItem;
}; // class TreeItem

#endif // TREEITEM_H
