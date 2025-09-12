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

    enum Tabs { TabListed, TabFilter, TabVerification, TabChanges, TabComment, TabAutoSelect = 1000 };

    void setCurrentTab(Tabs tab);
    QString getComment() const;

private:
    QStringList infoContent();
    QStringList infoVerification();
    QStringList infoChanges();

    void connections();
    void setTabsInfo();
    void setVisibleTabs();
    void setLabelsInfo();
    void selectCurTab();

    bool isCreating() const;
    bool isJustCreated() const;

    /*** Vars ***/
    Ui::DialogDbStatus *m_ui;
    const DataContainer *m_data = nullptr;

    // automatic selection of the current tab during execution
    bool autoTabSelection = true;

protected:
    void showEvent(QShowEvent *event) override;
}; // class DialogDbStatus

#endif // DIALOGDBSTATUS_H
