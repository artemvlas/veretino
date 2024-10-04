#ifndef TREEWIDGETITEM_H
#define TREEWIDGETITEM_H

#include <QTreeWidgetItem>

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

#endif // TREEWIDGETITEM_H
