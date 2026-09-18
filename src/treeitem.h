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
    enum ItemDataRoles : quint8 { FallbackRole = 100 };

    struct Data {
        Data() {}
        Data(const QVariant &data, quint8 role = Qt::DisplayRole) : _data{{role, data}} {}
        //explicit Data(const QString &str) : _data{{Qt::DisplayRole, str}} {}
        void setData(const QVariant &data, quint8 role = Qt::DisplayRole) { _data.insert(role, data); }
        QVariant data(quint8 role = Qt::DisplayRole) const { return _data.value(role); }

        Data &operator=(const QString &str) {
            setData(str);
            return *this;
        }

        // var
        QHash<quint8, QVariant> _data;
    }; // struct Data

    explicit TreeItem(const QVector<Data> &data, TreeItem *parent = nullptr);
    ~TreeItem();

    TreeItem *child(int number);
    TreeItem *parent();
    int childNumber() const;
    int childCount() const;
    int columnCount() const;
    QVariant data(int column, quint8 role = Qt::DisplayRole) const;
    bool setData(int column, const QVariant &value, quint8 role = Qt::DisplayRole);
    void appendChild(TreeItem *child);

    // creates and appends a new child item, returns a pointer to it
    TreeItem *addChild(const QVector<Data> &tiData);

    // looks for matches in column 0
    TreeItem *findChild(const QString &str) const;

private:
    QList<TreeItem*> childItems;
    QVector<Data> itemData;
    TreeItem *parentItem;
}; // class TreeItem

#endif // TREEITEM_H
