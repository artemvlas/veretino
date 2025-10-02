/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef ITEMFILETYPE_H
#define ITEMFILETYPE_H

#include <QTreeWidgetItem>
#include "nums.h"

class ItemFileType : public QTreeWidgetItem
{
public:
    ItemFileType(QTreeWidget* parent);

    enum Column {
        ColumnType,
        ColumnFilesNumber,
        ColumnTotalSize
    };

    enum TypeAttribute {
        NotSet = 0,
        UnCheckable = 1,  // enabling the checkbox is prohibited
        UnFilterable = 2, // values ​​do not count toward the filtered list
    };

    // toggle checkbox status (checked or unchecked)
    void toggle();
    void setChecked(bool checked);
    void setCheckBoxVisible(bool visible);

    // TypeAttribute or combined value
    void setAttribute(int attr);
    bool isChecked() const;
    bool isCheckBoxVisible() const;
    bool hasAttribute(TypeAttribute attr) const;

    // total number of files
    int filesNumber() const;

    // total size of this type files
    qint64 filesSize() const;

    // files number and their total size
    NumSize numSize() const;

    // --> visible type: file extension (suffix) or combined ("SymLinks", "Veretino DB", etc..)
    QString extension() const;

private:
    bool operator <(const QTreeWidgetItem &other) const override;
}; // class ItemFileType

using TypeAttribute = ItemFileType::TypeAttribute;

#endif // ITEMFILETYPE_H
