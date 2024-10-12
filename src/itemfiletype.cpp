/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "itemfiletype.h"

ItemFileType::ItemFileType(QTreeWidget* parent)
    : QTreeWidgetItem(parent) {}

QString ItemFileType::extension() const
{
    return data(ColumnType, Qt::DisplayRole).toString();
}

int ItemFileType::filesNumber() const
{
    return data(ColumnFilesNumber, Qt::DisplayRole).toInt();
}

qint64 ItemFileType::filesSize() const
{
    return data(ColumnTotalSize, Qt::UserRole).toLongLong();
}

NumSize ItemFileType::numSize() const
{
    return NumSize(filesNumber(), filesSize());
}

void ItemFileType::setChecked(bool checked)
{
    setCheckState(ColumnType, checked ? Qt::Checked : Qt::Unchecked);
}

void ItemFileType::setCheckBoxVisible(bool visible)
{
    visible ? setChecked(false) : setData(ColumnType, Qt::CheckStateRole, QVariant());
}

bool ItemFileType::isChecked() const
{
    QVariant checkState = data(ColumnType, Qt::CheckStateRole);
    return (checkState.isValid() && checkState == Qt::Checked);
}

bool ItemFileType::isCheckBoxVisible() const
{
    return data(ColumnType, Qt::CheckStateRole).isValid();
}

void ItemFileType::toggle()
{
    if (isCheckBoxVisible())
        setChecked(!isChecked());
}

bool ItemFileType::operator <(const QTreeWidgetItem &other) const
{
    int column = treeWidget()->sortColumn();
    if (column == ColumnTotalSize)
        return data(column, Qt::UserRole).toLongLong() < other.data(column, Qt::UserRole).toLongLong();

    return QTreeWidgetItem::operator <(other);
} // class ItemFileType
