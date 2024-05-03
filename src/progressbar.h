/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <QProgressBar>
#include <QTimer>
#include <QElapsedTimer>
#include "procstate.h"

class ProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    explicit ProgressBar(QWidget *parent = nullptr);
    void setProcState(const ProcState *proc);
    void setProgEnabled(bool enabled);

private:
    void updateProgressInfo();
    void updateDonePiece();
    QString progTimeLeft();
    QString progSpeed();

    const ProcState *procState_ = nullptr;
    QTimer *timer = new QTimer(this);

    QElapsedTimer elapsedTimer;
    qint64 pieceTime_; // milliseconds
    qint64 pieceSize_;
};

#endif // PROGRESSBAR_H
