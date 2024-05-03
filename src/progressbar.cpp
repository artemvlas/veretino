/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "progressbar.h"

ProgressBar::ProgressBar(QWidget *parent)
    : QProgressBar(parent)
{
    connect(timer, &QTimer::timeout, this, &ProgressBar::updateProgressInfo);
}

void ProgressBar::setProcState(ProcState *proc)
{
    procState_ = proc;
}

void ProgressBar::updateProgressInfo()
{
    if (procState_) {
        procState_->updateDonePiece();
        setFormat(QString("%p% | %1 | %2")
                      .arg(procState_->progSpeed(), procState_->progTimeLeft()));
    }
}

void ProgressBar::setProgEnabled(bool enabled)
{
    enabled ? timer->start(1000) : timer->stop();

    setVisible(enabled);
    setValue(0);
    resetFormat();
}
