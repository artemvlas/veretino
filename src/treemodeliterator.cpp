/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "treemodeliterator.h"

TreeModelIterator::TreeModelIterator(const QAbstractItemModel *model, QModelIndex rootIndex)
    : model_(model)
{
    rootIndex = (rootIndex.isValid() && rootIndex.model() == model) ? rootIndex.siblingAtColumn(Column::ColumnName)
                                                                    : QModelIndex();

    if (TreeModel::isFileRow(rootIndex))
        rootIndex = rootIndex.parent();

    if (rootIndex.isValid()) {
        rootIndex_ = rootIndex;
        index_ = rootIndex;
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

    return model_->hasChildren(next().index_) ? nextFile() : *this;
}

QModelIndex TreeModelIterator::stepForward(const QModelIndex &curIndex)
{
    if (endReached)
        return QModelIndex();

    if (model_->hasChildren(curIndex))
        return model_->index(0, 0, curIndex);

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
