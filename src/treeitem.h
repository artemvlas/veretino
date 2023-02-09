#ifndef TREEITEM_H
#define TREEITEM_H
#include "QVariant"

class TreeItem
{
public:
    explicit TreeItem(const QStringList &data, TreeItem *parentItem = nullptr);
    ~TreeItem();

    void appendChild(TreeItem *child);

    TreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QString data(int column) const;
    int row() const;
    TreeItem *parentItem();

private:
    QList<TreeItem *> m_childItems;
    QStringList m_itemData;
    TreeItem *m_parentItem;
};

#endif // TREEITEM_H
