/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef FOLDERCONTENTSDIALOG_H
#define FOLDERCONTENTSDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include "files.h"

class TreeWidgetItem : public QTreeWidgetItem
{
public:
    TreeWidgetItem(QTreeWidget* parent) : QTreeWidgetItem(parent){}
    enum Column {ColumnExtension, ColumnFilesNumber, ColumnTotalSize};

    QString extension() const
    {
        return data(ColumnExtension, Qt::DisplayRole).toString();
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
        checked ? setCheckState(ColumnExtension, Qt::Checked) : setCheckState(ColumnExtension, Qt::Unchecked);
    }

    void setCheckBoxVisible(bool visible)
    {
        visible ? setChecked(false) : setData(ColumnExtension, Qt::CheckStateRole, QVariant());
    }

    bool isChecked()
    {
        QVariant checkState = data(0, Qt::CheckStateRole);
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
class FolderContentsDialog;
}

class FolderContentsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FolderContentsDialog(const QString &folderPath, const QList<ExtNumSize> &extList, QWidget *parent = nullptr);
    ~FolderContentsDialog();
    FilterRule resultFilter();
    void setFilterCreatingEnabled(bool enabled = true);
protected:
    void showEvent(QShowEvent*) override; // for centering
private:
    Ui::FolderContentsDialog *ui;
    void connections();
    void setTotalInfo();
    void makeItemsList(const QList<ExtNumSize> &extList);
    void setItemsVisibility(bool isTop10Checked);
    void enableFilterCreating();
    void disableFilterCreating();
    void handleDoubleClickedItem(QTreeWidgetItem *item);
    void updateFilterExtensionsList();
    void updateTotalFiltered();
    bool isFilterCreatingEnabled();

    QList<ExtNumSize> extList_;
    QList<TreeWidgetItem *> items;
    QStringList filterExtensions;

}; // class FolderContentsDialog

#endif // FOLDERCONTENTSDIALOG_H
