/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "progressbar.h"

ProgressBar::ProgressBar(QWidget *parent)
    : QProgressBar(parent)
{
    connect(this, &ProgressBar::valueChanged, this, &ProgressBar::updateProgressInfo);
}

void ProgressBar::setProcState(const ProcState *proc)
{
    procState_ = proc;
}

void ProgressBar::updateProgressInfo()
{
    if (procState_) {
        setFormat(QString("%1% | %2 | %3")
                      .arg(value())
                      .arg(procState_->progSpeed(), procState_->progTimeLeft()));
    }
}
