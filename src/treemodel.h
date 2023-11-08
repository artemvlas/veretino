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

    QVariant data(const QModelIndex &curIndex, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &curIndex) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &curIndex) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool setData(const QModelIndex &curIndex, const QVariant &value,
                        int role = Qt::EditRole) override;

    bool isEmpty();

    void populate(const FileList &filesData);
    void setItemStatus(const QString &itemPath, FileValues::FileStatus status);

public slots:
    bool addFile(const QString &filePath, const FileValues &values);

private:
    TreeItem *getItem(const QModelIndex &curIndex) const;
    TreeItem *rootItem;
};

#endif // TREEMODEL_H
