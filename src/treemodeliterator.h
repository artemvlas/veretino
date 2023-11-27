// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef TREEMODELITERATOR_H
#define TREEMODELITERATOR_H
#include <QAbstractItemModel>
#include "tools.h"
#include "treemodel.h"

class TreeModelIterator
{
public:
    TreeModelIterator(const QAbstractItemModel *model, QModelIndex rootIndex = QModelIndex());
    TreeModelIterator& next();
    TreeModelIterator& nextFile();
    bool hasNext();
    const QModelIndex& index();
    QVariant data(Column column = Column::ColumnPath, int role = TreeModel::RawDataRole);
    QString path();
    FileStatus status();

private:
    QModelIndex nextRow(const QModelIndex &curIndex);
    QModelIndex stepForward(const QModelIndex &curIndex);

    const QAbstractItemModel *model_;
    QModelIndex index_;
    QModelIndex rootIndex_;
    QModelIndex nextIndex_;
};

#endif // TREEMODELITERATOR_H
