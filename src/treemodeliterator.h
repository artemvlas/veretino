#ifndef TREEMODELITERATOR_H
#define TREEMODELITERATOR_H
#include "treemodel.h"

class TreeModelIterator
{
public:
    TreeModelIterator(const TreeModel *model, QModelIndex rootIndex = QModelIndex());
    QModelIndex next();
    QModelIndex nextFile();
    bool hasNext();
    QString path();

    QModelIndex index_;

private:
    QModelIndex nextRow(const QModelIndex &curIndex);
    QModelIndex stepForward(const QModelIndex &curIndex);

    const TreeModel *model_;
    QModelIndex rootIndex_;
    QModelIndex nextIndex_;
};

#endif // TREEMODELITERATOR_H
