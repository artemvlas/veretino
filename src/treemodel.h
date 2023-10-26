// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include "treeitem.h"
#include "files.h"

class TreeItem;

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
    bool setData(const QModelIndex &index, const QVariant &value,
                        int role = Qt::EditRole) override;
    void addFile(const QString &filePath, const FileValues &values);
    void populate(const FileList &filesData);
    QString getPath(const QModelIndex &curIndex) const; // build path by current index data
    QModelIndex getIndex(const QString &path); // find index of specified 'path'
    void setItemStatus(const QString &itemPath, int status);


private:
    TreeItem *getItem(const QModelIndex &index) const;
    TreeItem *rootItem;
};

#endif // TREEMODEL_H
