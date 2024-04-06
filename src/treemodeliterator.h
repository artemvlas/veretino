/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
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
    QVariant data(Column column = Column::ColumnName, int role = TreeModel::RawDataRole);
    QString path(const QModelIndex &root = QModelIndex());
    FileStatus status();

private:
    QModelIndex nextRow(const QModelIndex &curIndex);
    QModelIndex stepForward(const QModelIndex &curIndex);

    const QAbstractItemModel *model_;
    QModelIndex index_;
    QModelIndex rootIndex_;
    QModelIndex nextIndex_; // the next index is found in advance and used as a cache to avoid calling the function twice when hasNext()

    bool endReached = false;
};

#endif // TREEMODELITERATOR_H
