/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef WIDGETFILETYPES_H
#define WIDGETFILETYPES_H

#include <QTreeWidget>
#include "files.h"
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

    // checkboxes are enabled and at least one is checked
    bool hasChecked() const;

    // show hidden ones
    void showAllItems();

    // leave only this number of items visible
    void hideExtra(int max_visible = 10);

    // total number and size of files (visible types only)
    NumSize numSizeVisible() const;

    // total number and size of listed files (all types)
    NumSize numSize(CheckState chk_state) const;

private:
    /*
     * create and add new Item to 'this' and the 'm_items' list
     * 'type' is file extension (suffix) or combined name
     * attribute is TypeAttribute or combined value
     */
    ItemFileType* addItem(const QString &type,
                          const NumSize &nums,
                          const QIcon &icon = QIcon(),
                          int attribute = 0);

    QList<ItemFileType*> m_items;
    IconProvider m_icons;

}; // class WidgetFileTypes

#endif // WIDGETFILETYPES_H
