// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "proxymodel.h"
#include "tools.h"
#include <QDebug>
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

ProxyModel::~ProxyModel()
{
    qDebug() << this << "deleted:";
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
    if (!isFilterEnabled || fileStatuses_.isEmpty())
        return true;

    QModelIndex curIndex = sourceModel()->index(sourceRow, Column::ColumnStatus, sourceParent);

    return fileStatuses_.contains(curIndex.data(TreeModel::RawDataRole).value<FileStatus>());
}

bool ProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    // folder + folder || file + file
    if ((sourceModel()->hasChildren(left) && sourceModel()->hasChildren(right))
        || (!sourceModel()->hasChildren(left) && !sourceModel()->hasChildren(right)))
        return QSortFilterProxyModel::lessThan(left, right);

    // folder + file
    return sourceModel()->hasChildren(left);
}

void ProxyModel::setFilter(QSet<FileStatus> statuses)
{
    if (statuses.isEmpty()) {
        disableFilter();
        return;
    }

    fileStatuses_ = statuses;
    isFilterEnabled = true;
    invalidateFilter();
}

void ProxyModel::disableFilter()
{
    isFilterEnabled = false;
    fileStatuses_.clear();
    invalidateFilter();
}

QSet<FileStatus> ProxyModel::currentlyFiltered()
{
    return fileStatuses_;
}
