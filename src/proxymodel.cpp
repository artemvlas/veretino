#include "proxymodel.h"
#include <QDebug>

ProxyModel::ProxyModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setRecursiveFilteringEnabled(true);
    setSortRole(1000); // TreeModel::RawDataRole = 1000
}

ProxyModel::~ProxyModel()
{
    qDebug() << this << "deleted:";
}

bool ProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!isFilterEnabled || fileStatuses_.isEmpty())
        return true;

    QModelIndex curIndex = sourceModel()->index(sourceRow, 2, sourceParent);

    return fileStatuses_.contains(curIndex.data(1000).toInt()); // TreeModel::RawDataRole = 1000
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

void ProxyModel::setFilter(QList<int> status)
{
    if (status.isEmpty()) {
        disableFilter();
        return;
    }

    fileStatuses_ = status;
    isFilterEnabled = true;
    invalidateFilter();
}

void ProxyModel::disableFilter()
{
    isFilterEnabled = false;
    fileStatuses_.clear();
    invalidateFilter();
}
