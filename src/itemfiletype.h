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
    }; // enum TypeAttribute

    void toggle();                                                     // toggle checkbox status (checked or unchecked)
    void setChecked(bool checked);
    void setCheckBoxVisible(bool visible);
    void setAttribute(int attr);                                       // TypeAttribute or combined value
    bool isChecked() const;
    bool isCheckBoxVisible() const;
    bool hasAttribute(TypeAttribute attr) const;
    int filesNumber() const;                                           // total number of files
    qint64 filesSize() const;                                          // total size of this type files
    NumSize numSize() const;                                           // files number and their total size
    QString extension() const;                                         // --> visible type: file extension (suffix) or combined ("SymLinks", "Veretino DB", etc..)

private:
    bool operator <(const QTreeWidgetItem &other) const override;
}; // class ItemFileType

using TypeAttribute = ItemFileType::TypeAttribute;

#endif // ITEMFILETYPE_H
