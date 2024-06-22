/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>
#include "clickablelabel.h"

class StatusBar : public QStatusBar
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget *parent = nullptr);
    ~StatusBar();

    ClickableLabel *statusIconLabel = new ClickableLabel(this);
    ClickableLabel *statusTextLabel = new ClickableLabel(this);
    ClickableLabel *permanentStatus = new ClickableLabel(this);
}; // class StatusBar

#endif // STATUSBAR_H
