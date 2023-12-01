// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef PROXYMODEL_H
#define PROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QSet>
#include "files.h"
#include "tools.h"

class ProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ProxyModel(QObject *parent = nullptr);
    ProxyModel(QAbstractItemModel *sourceModel, QObject *parent = nullptr);
    ~ProxyModel();
    void setFilter(QSet<FileStatus> statuses);
    void disableFilter();
    QSet<FileStatus> currentlyFiltered();
    bool isFilterEnabled = false;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    void setInitSettings();
    QSet<FileStatus> fileStatuses_;
};

#endif // PROXYMODEL_H
