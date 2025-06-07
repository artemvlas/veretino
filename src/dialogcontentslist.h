/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DIALOGCONTENTSLIST_H
#define DIALOGCONTENTSLIST_H

#include <QDialog>
#include <QKeyEvent>
#include "files.h"

namespace Ui {
class DialogContentsList;
}

class DialogContentsList : public QDialog
{
    Q_OBJECT

public:
    explicit DialogContentsList(const QString &folderPath,
                                const FileTypeList &extList,
                                QWidget *parent = nullptr);
    ~DialogContentsList();

private:
    Ui::DialogContentsList *ui;

    void connections();
    void setTotalInfo(const FileTypeList &extList);
    void setItemsVisibility(bool isTop10Checked);
    void updateSelectInfo();

    NumSize _n_total;
}; // class DialogContentsList

#endif // DIALOGCONTENTSLIST_H
