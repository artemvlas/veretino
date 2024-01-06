#ifndef FOLDERCONTENTSDIALOG_H
#define FOLDERCONTENTSDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include "files.h"

namespace Ui {
class FolderContentsDialog;
}

class FolderContentsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FolderContentsDialog(const QString &folderName, const QList<ExtNumSize> &extList, QWidget *parent = nullptr);
    ~FolderContentsDialog();
    enum ColumnFCD {ColumnExtension, ColumnFilesNumber, ColumnTotalSize};

private:
    Ui::FolderContentsDialog *ui;
    void setTotalInfo();
    void addItemToTreeWidget(const ExtNumSize &itemData);
    void populate();

    QList<ExtNumSize> extList_;

}; // class FolderContentsDialog

class TreeWidgetItem : public QTreeWidgetItem
{
public:
    TreeWidgetItem(QTreeWidget* parent) : QTreeWidgetItem(parent){}
private:
    bool operator <(const QTreeWidgetItem &other)const override
    {
        int column = treeWidget()->sortColumn();
        if (column == FolderContentsDialog::ColumnTotalSize)
            return data(column, Qt::UserRole).toLongLong() < other.data(column, Qt::UserRole).toLongLong();

        return QTreeWidgetItem::operator <(other);
    }
}; // class TreeWidgetItem

#endif // FOLDERCONTENTSDIALOG_H
