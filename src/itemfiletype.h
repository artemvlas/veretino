/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef ITEMFILETYPE_H
#define ITEMFILETYPE_H

#include <QTreeWidgetItem>
#include "files.h" // for NumSize

class ItemFileType : public QTreeWidgetItem
{
public:
    ItemFileType(QTreeWidget* parent);
    enum Column { ColumnType, ColumnFilesNumber, ColumnTotalSize };
    enum TypeAttribute {
        NotSet = 0,
        UnCheckable = 1,  // enabling the checkbox is prohibited
        UnFilterable = 2, // values ​​do not count toward the filtered list
    };

    void toggle();
    void setChecked(bool checked);
    void setCheckBoxVisible(bool visible);
    bool isChecked() const;
    bool isCheckBoxVisible() const;
    bool hasAttribute(TypeAttribute attr) const;
    int filesNumber() const;
    qint64 filesSize() const;
    NumSize numSize() const;
    QString extension() const;

private:
    bool operator <(const QTreeWidgetItem &other) const override;
}; // class ItemFileType

using TypeAttribute = ItemFileType::TypeAttribute;

#endif // ITEMFILETYPE_H
