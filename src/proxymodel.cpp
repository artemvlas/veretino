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

    QModelIndex _ind = sourceModel()->index(sourceRow, Column::ColumnStatus, sourceParent);
    FileStatus _status = _ind.data(TreeModel::RawDataRole).value<FileStatus>();

    return (_status & filteredFlags);
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
    if (flags != filteredFlags) {
        filteredFlags = flags;
        invalidateFilter();
    }
}

void ProxyModel::disableFilter()
{
    if (isFilterEnabled()) {
        filteredFlags = FileStatus::NotSet;
        invalidateFilter();
    }
}

FileStatuses ProxyModel::currentlyFiltered() const
{
    return filteredFlags;
}

bool ProxyModel::isFilterEnabled() const
{
    return (filteredFlags != FileStatus::NotSet);
}
