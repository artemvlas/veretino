/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "progressbar.h"
#include "tools.h"

ProgressBar::ProgressBar(QWidget *parent)
    : QProgressBar(parent)
{
    connect(timer, &QTimer::timeout, this, &ProgressBar::updateProgressInfo);
}

void ProgressBar::setProcState(const ProcState *proc)
{
    procState_ = proc;
}

void ProgressBar::start()
{
    setProgEnabled(true);
}

void ProgressBar::finish()
{
    setProgEnabled(false);
}

void ProgressBar::setProgEnabled(bool enabled)
{
    if (enabled) {
        timer->start(1000);
        elapsedTimer.start();
    }
    else
        timer->stop();

    resetFormat();
    setValue(0);
    setVisible(enabled);
}

void ProgressBar::updateProgressInfo()
{
    if (procState_) {
        updateDonePiece();
        setFormat(QString("%p% | %1 | %2")
                      .arg(progSpeed(), progTimeLeft()));
    }
}

void ProgressBar::updateDonePiece()
{
    pieceTime_ = elapsedTimer.restart();
    pieceSize_ = procState_->donePieceSize();
}

QString ProgressBar::progTimeLeft()
{
    QString result;

    if (pieceSize_ > 0) {
        qint64 timeleft = (procState_->remainingSize() / pieceSize_) * pieceTime_;
        result = format::millisecToReadable(timeleft, true);
    }

    return result;
}

QString ProgressBar::progSpeed()
{
    QString result;

    if (pieceTime_ > 0) {
        result = QString("%1/s")
                     .arg(format::dataSizeReadable((pieceSize_ / pieceTime_) * 1000));
    }

    return result;
}
