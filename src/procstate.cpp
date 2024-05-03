/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "procstate.h"
#include "tools.h"

ProcState::ProcState(QObject *parent)
    : QObject{parent}
{}

void ProcState::setTotalSize(qint64 totalSize)
{
    totalSize_ = totalSize;
    doneSize_ = 0;
}

void ProcState::addChunk(int chunk)
{
    if (doneSize_ == 0)
        startProgress();

    toPercents(chunk);
}

qint64 ProcState::doneSize() const
{
    return doneSize_;
}

void ProcState::startProgress()
{
    prevTimePassed = 0;
    prevDoneSize = 0;
    elapsedTimer.start();
    emit percentageChanged(0); // initial 0 to reset progressbar value
}

void ProcState::toPercents(int bytes)
{
    if (totalSize_ == 0)
        return;

    int lastPerc = (doneSize_ * 100) / totalSize_; // before current chunk added

    doneSize_ += bytes;
    int curPerc = (doneSize_ * 100) / totalSize_; // after

    if (curPerc > lastPerc) {
        updateDonePiece();
        emit percentageChanged(curPerc);
    }
}

void ProcState::updateDonePiece()
{
    pieceTime = elapsedTimer.elapsed() - prevTimePassed; // milliseconds
    pieceSize = doneSize_ - prevDoneSize;
    prevTimePassed += pieceTime;
    prevDoneSize += pieceSize;
}

QString ProcState::progTimeLeft() const
{
    QString result;

    if (pieceSize > 0) {
        qint64 timeleft = ((totalSize_ - doneSize_) / pieceSize) * pieceTime;
        result = format::millisecToReadable(timeleft, true);
    }

    return result;
}

QString ProcState::progSpeed() const
{
    QString result;

    if (pieceTime > 0) {
        result = QString("%1/sec")
                 .arg(format::dataSizeReadable((pieceSize / pieceTime) * 1000));
    }

    return result;
}
