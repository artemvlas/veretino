/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef PROXYMODEL_H
#define PROXYMODEL_H

#include <QSortFilterProxyModel>
#include "files.h"
#include "tools.h"

class ProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ProxyModel(QObject *parent = nullptr);
    ProxyModel(QAbstractItemModel *sourceModel, QObject *parent = nullptr);
    ~ProxyModel();
    void setFilter(const FileStatuses flags);
    void disableFilter();
    FileStatuses currentlyFiltered() const;
    bool isFilterEnabled() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    void setInitSettings();
    FileStatuses filteredFlags = FileStatus::NotSet;
};

#endif // PROXYMODEL_H
