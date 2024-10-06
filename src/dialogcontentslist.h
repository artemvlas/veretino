/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DIALOGCONTENTSLIST_H
#define DIALOGCONTENTSLIST_H

#include <QDialog>
#include <QKeyEvent>
#include "files.h"
#include "iconprovider.h"
#include "itemfiletype.h"

namespace Ui {
class DialogContentsList;
}

class DialogContentsList : public QDialog
{
    Q_OBJECT
public:
    explicit DialogContentsList(const QString &folderPath, const FileTypeList &extList, QWidget *parent = nullptr);
    ~DialogContentsList();

    enum FilterCreation { FC_Hidden, FC_Disabled, FC_Enabled };
    void setFilterCreation(FilterCreation mode);
    FilterRule resultFilter();

private:
    Ui::DialogContentsList *ui;
    enum CheckState { Checked, UnChecked };
    void connections();
    void setTotalInfo();
    void makeItemsList(const FileTypeList &extList);
    void setItemsVisibility(bool isTop10Checked);
    void setCheckboxesVisible(bool visible);
    void clearChecked();
    void updateViewMode();
    void activateItem(QTreeWidgetItem *t_item);
    void updateFilterDisplay();
    void updateLabelFilterExtensions();
    void updateLabelTotalFiltered();
    bool isPassedChecked(const ItemFileType *item) const;
    bool isPassedUnChecked(const ItemFileType *item) const;
    bool isPassed(CheckState state, const ItemFileType *item) const;
    bool itemsContain(CheckState state) const;
    QList<ItemFileType *> items(CheckState state) const;
    QStringList checkedExtensions() const;

    FileTypeList extList_;
    QList<ItemFileType *> items_;

    IconProvider icons_;
    FilterCreation mode_ = FC_Hidden;

protected:
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;
}; // class DialogContentsList

#endif // DIALOGCONTENTSLIST_H
