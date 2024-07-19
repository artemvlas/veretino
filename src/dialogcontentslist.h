/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DIALOGCONTENTSLIST_H
#define DIALOGCONTENTSLIST_H

#include <QDialog>
#include <QTreeWidgetItem>
#include <QKeyEvent>
#include "files.h"
#include "iconprovider.h"

class TreeWidgetItem : public QTreeWidgetItem
{
public:
    TreeWidgetItem(QTreeWidget* parent);
    enum Column { ColumnType, ColumnFilesNumber, ColumnTotalSize };

    void toggle();
    void setChecked(bool checked);
    void setCheckBoxVisible(bool visible);
    bool isChecked() const;
    bool isCheckBoxVisible() const;
    int filesNumber() const;
    qint64 filesSize() const;
    QString extension() const;

private:
    bool operator <(const QTreeWidgetItem &other) const override;
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
    enum CheckState { Checked, UnChecked };
    void connections();
    void setTotalInfo();
    void makeItemsList(const QList<ExtNumSize> &extList);
    void setItemsVisibility(bool isTop10Checked);
    void setCheckboxesVisible(bool visible);
    void clearChecked();
    void updateViewMode();
    void activateItem(QTreeWidgetItem *t_item);
    void updateFilterDisplay();
    void updateLabelFilterExtensions();
    void updateLabelTotalFiltered();
    bool isPassedChecked(const TreeWidgetItem *item) const;
    bool isPassedUnChecked(const TreeWidgetItem *item) const;
    bool isPassed(CheckState state, const TreeWidgetItem *item) const;
    bool itemsContain(CheckState state) const;
    QList<TreeWidgetItem *> items(CheckState state) const;
    QStringList checkedExtensions() const;

    QList<ExtNumSize> extList_;
    QList<TreeWidgetItem *> items_;

    IconProvider icons_;
    FilterCreation mode_ = FC_Hidden;

protected:
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;
}; // class DialogContentsList

#endif // DIALOGCONTENTSLIST_H
