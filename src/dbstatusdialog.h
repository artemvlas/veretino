/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef DBSTATUSDIALOG_H
#define DBSTATUSDIALOG_H

#include <QDialog>
#include "datacontainer.h"

namespace Ui {
class DbStatusDialog;
}

class DbStatusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DbStatusDialog(const DataContainer *data, QWidget *parent = nullptr);
    ~DbStatusDialog();
private:
    Ui::DbStatusDialog *ui;
    const DataContainer *data_;

    enum Tabs{TabListed, TabFilter, TabVerification, TabChanges};
    QStringList infoContent(const DataContainer *data);
    QStringList infoVerification(const DataContainer *data);
    QStringList infoChanges();
    bool isJustCreated();
};

#endif // DBSTATUSDIALOG_H
