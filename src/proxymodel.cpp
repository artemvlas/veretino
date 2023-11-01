#include "proxymodel.h"
#include "tools.h"
#include <QDebug>

ProxyModel::ProxyModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setRecursiveFilteringEnabled(true);
    setSortRole(ModelKit::RawDataRole);
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

    return fileStatuses_.contains(curIndex.data(ModelKit::RawDataRole).toInt());
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
