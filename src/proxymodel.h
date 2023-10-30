#ifndef PROXYMODEL_H
#define PROXYMODEL_H

#include <QSortFilterProxyModel>

class ProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ProxyModel(QObject *parent = nullptr);
    ~ProxyModel();
    void setFilter(QList<int> status);
    void disableFilter();
    bool isFilterEnabled = false;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QList<int> fileStatuses_;
};

#endif // PROXYMODEL_H
