/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef DIALOGDBSTATUS_H
#define DIALOGDBSTATUS_H

#include <QDialog>
#include "datacontainer.h"

namespace Ui {
class DialogDbStatus;
}

class DialogDbStatus : public QDialog
{
    Q_OBJECT

public:
    explicit DialogDbStatus(const DataContainer *data, QWidget *parent = nullptr);
    ~DialogDbStatus();
private:
    Ui::DialogDbStatus *ui;
    const DataContainer *data_;

    enum Tabs{TabListed, TabFilter, TabVerification, TabChanges};
    QStringList infoContent(const DataContainer *data);
    QStringList infoVerification(const DataContainer *data);
    QStringList infoChanges();
    bool isJustCreated();
};

#endif // DIALOGDBSTATUS_H
