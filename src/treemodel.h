#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include "treeitem.h"
#include "datacontainer.h"

class TreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit TreeModel(QObject *parent = nullptr);
    ~TreeModel();

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    void populate(const FileList &filesData);

private:
    TreeItem *rootItem;
    QString getPath(const QModelIndex &index);
};

#endif // TREEMODEL_H
