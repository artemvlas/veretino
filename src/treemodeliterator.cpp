#include "treemodeliterator.h"

TreeModelIterator::TreeModelIterator(const TreeModel *model, QModelIndex rootIndex)
    : model_(model)
{
    if (rootIndex.isValid()) {
        rootIndex = model->index(rootIndex.row(), 0, rootIndex.parent());
        if (!model_->hasChildren(rootIndex))
            rootIndex = rootIndex.parent();
        if (rootIndex.isValid()) {
            rootIndex_ = rootIndex;
            index_ = rootIndex;
        }
    }

    nextIndex_ = stepForward(index_);
}

QModelIndex TreeModelIterator::next()
{
    index_ = nextIndex_;
    nextIndex_ = stepForward(index_);
    return index_;
}

QModelIndex TreeModelIterator::nextFile()
{
    return model_->hasChildren(next()) ? nextFile() : index_;
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

QString TreeModelIterator::path()
{
    return model_->getPath(index_);
}

bool TreeModelIterator::hasNext()
{
    return nextIndex_.isValid();
}
