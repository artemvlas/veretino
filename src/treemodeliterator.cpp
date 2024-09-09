/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "treemodeliterator.h"

TreeModelIterator::TreeModelIterator(const QAbstractItemModel *model, const QModelIndex &root)
    : modelConst_(model)
{
    setup(root);
}

void TreeModelIterator::setup(const QModelIndex &root)
{
    if (root.isValid() && root.model() == modelConst_) {
        const QModelIndex &_ind = TreeModel::isFileRow(root) ? root.parent()
                                                             : root.siblingAtColumn(Column::ColumnName);

        if (_ind.isValid()) { // subfolder
            rootIndex_ = _ind;
            index_ = _ind;
        }
    }

    nextIndex_ = stepForward(index_);
}

bool TreeModelIterator::hasNext() const
{
    return nextIndex_.isValid();
}

TreeModelIterator& TreeModelIterator::next()
{
    index_ = nextIndex_;
    nextIndex_ = stepForward(index_);
    return *this;
}

TreeModelIterator& TreeModelIterator::nextFile()
{
    if (endReached)
        return next();

    return modelConst_->hasChildren(next().index_) ? nextFile() : *this;
}

QModelIndex TreeModelIterator::stepForward(const QModelIndex &curIndex)
{
    if (endReached)
        return QModelIndex();

    if (modelConst_->hasChildren(curIndex))
        return modelConst_->index(0, 0, curIndex);

    QModelIndex&& _nextRow = nextRow(curIndex);
    if (_nextRow.isValid())
        return _nextRow;

    QModelIndex ind = curIndex;
    QModelIndex estimatedIndex;

    while (!estimatedIndex.isValid()) {
        ind = ind.parent();
        if (ind == rootIndex_)
            break;
        estimatedIndex = nextRow(ind);
    }

    endReached = !estimatedIndex.isValid();

    return estimatedIndex.isValid() ? estimatedIndex : QModelIndex();
}

QModelIndex TreeModelIterator::nextRow(const QModelIndex &curIndex) const
{
    return curIndex.siblingAtRow(curIndex.row() + 1);
}

const QModelIndex& TreeModelIterator::index() const
{
    return index_;
}

QVariant TreeModelIterator::data(Column column, int role) const
{
    return index_.siblingAtColumn(column).data(role);
}

QString TreeModelIterator::path(const QModelIndex &root) const
{
    return TreeModel::getPath(index_, root);
}

qint64 TreeModelIterator::size() const
{
    return data(Column::ColumnSize).toLongLong();
}

FileStatus TreeModelIterator::status() const
{
    return data(Column::ColumnStatus).value<FileStatus>();
}

QString TreeModelIterator::checksum() const
{
    return data(Column::ColumnChecksum).toString();
}
