/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
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

    enum Tabs{ TabListed, TabFilter, TabVerification, TabChanges };
    QStringList infoContent();
    QStringList infoVerification();
    QStringList infoChanges();

    void connections();
    void setTabsInfo();
    void setVisibleTabs();
    void setLabelsInfo();

    bool isJustCreated();
    bool isSavedToDesktop();
}; // class DialogDbStatus

#endif // DIALOGDBSTATUS_H
