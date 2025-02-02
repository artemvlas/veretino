/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "treemodeliterator.h"

TreeModelIterator::TreeModelIterator(const QAbstractItemModel *model, const QModelIndex &root)
    : m_modelConst(model)
{
    setup(root);
}

void TreeModelIterator::setup(const QModelIndex &root)
{
    if (root.isValid() && root.model() == m_modelConst) {
        const QModelIndex ind = TreeModel::isFileRow(root) ? root.parent()
                                                           : root.siblingAtColumn(Column::ColumnName);

        if (ind.isValid()) { // subfolder
            m_rootIndex = ind;
            m_index = ind;
        }
    }

    if (m_modelConst)
        m_nextIndex = stepForward(m_index);
    else
        m_endReached = true;
}

bool TreeModelIterator::hasNext() const
{
    // return m_nextIndex.isValid();
    return !m_endReached;
}

TreeModelIterator& TreeModelIterator::next()
{
    m_index = m_nextIndex;
    m_nextIndex = stepForward(m_index);
    return *this;
}

TreeModelIterator& TreeModelIterator::nextFile()
{
    if (m_endReached)
        return next();

    return m_modelConst->hasChildren(next().m_index) ? nextFile() : *this;
}

QModelIndex TreeModelIterator::stepForward(const QModelIndex &curIndex)
{
    if (m_endReached)
        return QModelIndex();

    if (m_modelConst->hasChildren(curIndex))
        return m_modelConst->index(0, 0, curIndex);

    QModelIndex ind_nextRow = nextRow(curIndex);
    if (ind_nextRow.isValid())
        return ind_nextRow;

    QModelIndex ind = curIndex;
    QModelIndex estimatedIndex;

    do {
        ind = ind.parent();
        if (ind == m_rootIndex)
            break;
        estimatedIndex = nextRow(ind);
    } while (!estimatedIndex.isValid());

    m_endReached = !estimatedIndex.isValid();

    return m_endReached ? QModelIndex() : estimatedIndex;
}

QModelIndex TreeModelIterator::nextRow(const QModelIndex &curIndex) const
{
    return curIndex.siblingAtRow(curIndex.row() + 1);
}

const QModelIndex& TreeModelIterator::index() const
{
    return m_index;
}

QVariant TreeModelIterator::data(Column column, int role) const
{
    return m_index.siblingAtColumn(column).data(role);
}

QString TreeModelIterator::path(const QModelIndex &root) const
{
    return TreeModel::getPath(m_index, root);
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
