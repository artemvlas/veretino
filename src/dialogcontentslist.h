/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DIALOGCONTENTSLIST_H
#define DIALOGCONTENTSLIST_H

#include <QDialog>
#include <QTreeWidgetItem>
#include "files.h"
#include "iconprovider.h"

class TreeWidgetItem : public QTreeWidgetItem
{
public:
    TreeWidgetItem(QTreeWidget* parent) : QTreeWidgetItem(parent){}
    enum Column { ColumnType, ColumnFilesNumber, ColumnTotalSize };

    QString extension() const
    {
        return data(ColumnType, Qt::DisplayRole).toString();
    }

    int filesNumber() const
    {
        return data(ColumnFilesNumber, Qt::DisplayRole).toInt();
    }

    qint64 filesSize() const
    {
        return data(ColumnTotalSize, Qt::UserRole).toLongLong();
    }

    void setChecked(bool checked)
    {
        checked ? setCheckState(ColumnType, Qt::Checked) : setCheckState(ColumnType, Qt::Unchecked);
    }

    void setCheckBoxVisible(bool visible)
    {
        visible ? setChecked(false) : setData(ColumnType, Qt::CheckStateRole, QVariant());
    }

    bool isChecked()
    {
        QVariant checkState = data(ColumnType, Qt::CheckStateRole);
        return (checkState.isValid() && checkState == Qt::Checked);
    }

private:
    bool operator <(const QTreeWidgetItem &other)const override
    {
        int column = treeWidget()->sortColumn();
        if (column == ColumnTotalSize)
            return data(column, Qt::UserRole).toLongLong() < other.data(column, Qt::UserRole).toLongLong();

        return QTreeWidgetItem::operator <(other);
    }
}; // class TreeWidgetItem

namespace Ui {
class DialogContentsList;
}

class DialogContentsList : public QDialog
{
    Q_OBJECT

public:
    explicit DialogContentsList(const QString &folderPath, const QList<ExtNumSize> &extList, QWidget *parent = nullptr);
    ~DialogContentsList();

    enum FilterCreation { FC_Hidden, FC_Disabled, FC_Enabled };
    void setFilterCreation(FilterCreation mode);
    FilterRule resultFilter();

private:
    Ui::DialogContentsList *ui;
    void connections();
    void setTotalInfo();
    void makeItemsList(const QList<ExtNumSize> &extList);
    void setItemsVisibility(bool isTop10Checked);
    void setCheckboxesVisible(bool visible);
    void updateViewMode();
    void enableFilterCreating();
    void disableFilterCreating();
    void handleDoubleClickedItem(QTreeWidgetItem *item);
    void updateFilterExtensionsList();
    void updateTotalFiltered();
    bool isFilterCreatingEnabled();
    bool isItemFilterable(const TreeWidgetItem *item);

    QList<ExtNumSize> extList_;
    QList<TreeWidgetItem *> items_;
    QStringList filterExtensions;

    IconProvider icons_;
    FilterCreation mode_ = FC_Hidden;

protected:
    void showEvent(QShowEvent *event) override;
}; // class DialogContentsList

#endif // DIALOGCONTENTSLIST_H
