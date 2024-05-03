/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <QProgressBar>
#include "procstate.h"

class ProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    explicit ProgressBar(QWidget *parent = nullptr);
    void setProcState(const ProcState *proc);

private:
    void updateProgressInfo();
    const ProcState *procState_ = nullptr;
};

#endif // PROGRESSBAR_H
