#include "treewidgetitem.h"

TreeWidgetItem::TreeWidgetItem(QTreeWidget* parent)
    : QTreeWidgetItem(parent) {}

QString TreeWidgetItem::extension() const
{
    return data(ColumnType, Qt::DisplayRole).toString();
}

int TreeWidgetItem::filesNumber() const
{
    return data(ColumnFilesNumber, Qt::DisplayRole).toInt();
}

qint64 TreeWidgetItem::filesSize() const
{
    return data(ColumnTotalSize, Qt::UserRole).toLongLong();
}

void TreeWidgetItem::setChecked(bool checked)
{
    setCheckState(ColumnType, checked ? Qt::Checked : Qt::Unchecked);
}

void TreeWidgetItem::setCheckBoxVisible(bool visible)
{
    visible ? setChecked(false) : setData(ColumnType, Qt::CheckStateRole, QVariant());
}

bool TreeWidgetItem::isChecked() const
{
    QVariant checkState = data(ColumnType, Qt::CheckStateRole);
    return (checkState.isValid() && checkState == Qt::Checked);
}

bool TreeWidgetItem::isCheckBoxVisible() const
{
    return data(ColumnType, Qt::CheckStateRole).isValid();
}

void TreeWidgetItem::toggle()
{
    if (isCheckBoxVisible())
        setChecked(!isChecked());
}

bool TreeWidgetItem::operator <(const QTreeWidgetItem &other) const
{
    int column = treeWidget()->sortColumn();
    if (column == ColumnTotalSize)
        return data(column, Qt::UserRole).toLongLong() < other.data(column, Qt::UserRole).toLongLong();

    return QTreeWidgetItem::operator <(other);
} // class TreeWidgetItem
