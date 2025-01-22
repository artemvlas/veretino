/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef WIDGETFILETYPES_H
#define WIDGETFILETYPES_H

#include <QTreeWidget>
#include "files.h"
#include "tools.h"
#include "iconprovider.h"
#include "itemfiletype.h"

class WidgetFileTypes : public QTreeWidget
{
public:
    WidgetFileTypes(QWidget *parent = nullptr);
    enum CheckState { UnChecked, Checked };

    void setItems(const FileTypeList &extList);
    void setCheckboxesVisible(bool visible);
    void setChecked(const QStringList &exts);
    void setChecked(const QSet<QString> &exts);
    QList<ItemFileType*> items(CheckState state) const;
    QStringList checkedExtensions() const;
    bool isPassedChecked(const ItemFileType *item) const;
    bool isPassedUnChecked(const ItemFileType *item) const;
    bool isPassed(CheckState state, const ItemFileType *item) const;
    bool itemsContain(CheckState state) const;
    bool hasChecked() const;
    void showAllItems();
    void hideExtra(int nomore = 10); // leave only this number of items visible
    NumSize numSizeVisible() const;
    NumSize numSize(CheckState chk_state) const;

    QList<ItemFileType*> m_items;

private:
    ItemFileType* addItem(const QString &type, const NumSize &nums, const QIcon &icon = QIcon());
    IconProvider icons_;

}; // class WidgetFileTypes

#endif // WIDGETFILETYPES_H
