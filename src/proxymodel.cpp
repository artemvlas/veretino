/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "proxymodel.h"
#include "treemodel.h"

ProxyModel::ProxyModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{
    setInitSettings();
}

ProxyModel::ProxyModel(QAbstractItemModel *sourceModel, QObject *parent)
    : QSortFilterProxyModel{parent}
{
    setInitSettings();
    setSourceModel(sourceModel);
}

void ProxyModel::setInitSettings()
{
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setRecursiveFilteringEnabled(true);
    setSortRole(TreeModel::RawDataRole);
    //setDynamicSortFilter(false);
}

bool ProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!isFilterEnabled())
        return true;

    QModelIndex ind = sourceModel()->index(sourceRow, Column::ColumnStatus, sourceParent);
    FileStatus status = ind.data(TreeModel::RawDataRole).value<FileStatus>();

    return (status & m_filteredFlags);
}

bool ProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    // folder + folder || file + file
    if ((sourceModel()->hasChildren(left) && sourceModel()->hasChildren(right))
        || (!sourceModel()->hasChildren(left) && !sourceModel()->hasChildren(right)))
    {
        return QSortFilterProxyModel::lessThan(left, right);
    }

    // folder + file
    return sourceModel()->hasChildren(left);
}

void ProxyModel::setFilter(const FileStatuses flags)
{
    if (flags != m_filteredFlags) {
        m_filteredFlags = flags;
        invalidateFilter();
    }
}

void ProxyModel::disableFilter()
{
    if (isFilterEnabled()) {
        m_filteredFlags = FileStatus::NotSet;
        invalidateFilter();
    }
}

FileStatuses ProxyModel::currentlyFiltered() const
{
    return m_filteredFlags;
}

bool ProxyModel::isFilterEnabled() const
{
    return (m_filteredFlags != FileStatus::NotSet);
}
