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

    void toggle();
    void setChecked(bool checked);
    void setCheckBoxVisible(bool visible);
    bool isChecked() const;
    bool isCheckBoxVisible() const;
    int filesNumber() const;
    qint64 filesSize() const;
    NumSize numSize() const;
    QString extension() const;

private:
    bool operator <(const QTreeWidgetItem &other) const override;
}; // class ItemFileType

#endif // ITEMFILETYPE_H
