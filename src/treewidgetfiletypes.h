#ifndef TREEWIDGETFILETYPES_H
#define TREEWIDGETFILETYPES_H

#include <QTreeWidget>
#include "files.h"
#include "iconprovider.h"
#include "treewidgetitem.h"

class TreeWidgetFileTypes : public QTreeWidget
{
public:
    TreeWidgetFileTypes(QWidget *parent = nullptr);
    enum CheckState { UnChecked, Checked };

    void setItems(const QList<ExtNumSize> &extList);
    void setCheckboxesVisible(bool visible);
    QList<TreeWidgetItem *> items(CheckState state) const;
    QStringList checkedExtensions() const;
    bool isPassedChecked(const TreeWidgetItem *item) const;
    bool isPassedUnChecked(const TreeWidgetItem *item) const;
    bool isPassed(CheckState state, const TreeWidgetItem *item) const;
    bool itemsContain(CheckState state) const;
    void showAllItems();

    QList<TreeWidgetItem *> items_;

private:
    IconProvider icons_;

}; // class TreeWidgetFileTypes

#endif // TREEWIDGETFILETYPES_H
