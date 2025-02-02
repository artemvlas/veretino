/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef TREEMODELITERATOR_H
#define TREEMODELITERATOR_H
#include <QAbstractItemModel>
#include "treemodel.h"

class TreeModelIterator
{
public:
    TreeModelIterator(const QAbstractItemModel *model, const QModelIndex &root = QModelIndex());
    TreeModelIterator& next();
    TreeModelIterator& nextFile();
    bool hasNext() const;
    const QModelIndex& index() const;
    QVariant data(Column column = Column::ColumnName, int role = TreeModel::RawDataRole) const;
    QString path(const QModelIndex &root = QModelIndex()) const;
    qint64 size() const;
    FileStatus status() const;
    QString checksum() const;

private:
    void setup(const QModelIndex &root);
    QModelIndex nextRow(const QModelIndex &curIndex) const;
    QModelIndex stepForward(const QModelIndex &curIndex);

    const QAbstractItemModel *m_modelConst;
    QModelIndex m_index;
    QModelIndex m_rootIndex;
    QModelIndex m_nextIndex;
    // the m_nextIndex is found in advance and used as a cache
    // to avoid calling the ::stepForward twice when hasNext() called

    bool m_endReached = false;
}; // class TreeModelIterator

#endif // TREEMODELITERATOR_H
