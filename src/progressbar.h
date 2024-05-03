/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <QProgressBar>
#include <QTimer>
#include "procstate.h"

class ProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    explicit ProgressBar(QWidget *parent = nullptr);
    void setProcState(ProcState *proc);
    void setProgEnabled(bool enabled);

private:
    void updateProgressInfo();
    ProcState *procState_ = nullptr;
    QTimer *timer = new QTimer(this);
};

#endif // PROGRESSBAR_H
