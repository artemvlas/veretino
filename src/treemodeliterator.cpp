// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "treemodeliterator.h"

TreeModelIterator::TreeModelIterator(const QAbstractItemModel *model, QModelIndex rootIndex)
    : model_(model)
{
    rootIndex = ModelKit::siblingAtRow(rootIndex, ModelKit::ColumnPath);

    if (ModelKit::isFileRow(rootIndex))
        rootIndex = rootIndex.parent();

    if (rootIndex.isValid()) {
        rootIndex_ = rootIndex;
        index_ = rootIndex;
    }

    nextIndex_ = stepForward(index_);
}

bool TreeModelIterator::hasNext()
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
    return model_->hasChildren(next().index_) ? nextFile() : *this;
}

QModelIndex TreeModelIterator::stepForward(const QModelIndex &curIndex)
{
    if (model_->hasChildren(curIndex))
        return model_->index(0, 0, curIndex);

    QModelIndex ind = nextRow(curIndex);
    if (ind.isValid())
        return ind;

    ind = curIndex;
    QModelIndex estimatedIndex;

    do {
        ind = ind.parent();
        if (ind == rootIndex_)
            break;
        estimatedIndex = nextRow(ind);
    } while (!estimatedIndex.isValid());

    return estimatedIndex.isValid() ? estimatedIndex : QModelIndex();
}

QModelIndex TreeModelIterator::nextRow(const QModelIndex &curIndex)
{
    return model_->index(curIndex.row() + 1, 0, curIndex.parent());
}

const QModelIndex& TreeModelIterator::index()
{
    return index_;
}

QVariant TreeModelIterator::data(ModelKit::Columns column, int role)
{
    return model_->data(model_->sibling(index_.row(), column, index_), role);
}

QString TreeModelIterator::path()
{
    return ModelKit::getPath(index_);
}

FileStatus TreeModelIterator::status()
{
    return data(ModelKit::ColumnStatus).value<FileStatus>();
}
