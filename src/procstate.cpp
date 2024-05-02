/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "procstate.h"
#include "tools.h"

ProcState::ProcState(qint64 totalSize, QObject *parent)
    : QObject{parent}, totalSize_(totalSize)
{
    connect(this, &ProcState::percentageChanged, this, &ProcState::sendProgressInfo);
}

void ProcState::addChunk(int chunk)
{
    toPercents(chunk);
}

qint64 ProcState::doneSize()
{
    return doneSize_;
}

void ProcState::startProgress()
{
    prevTimePassed = 0;
    prevDoneSize = 0;
    elapsedTimer.start();
}

void ProcState::sendProgressInfo(int percDone)
{
    if (percDone == 0)
        startProgress();
    else {
        donePiece();
        emit progressInfoChanged(QString("%1% | %2 | %3")
                                     .arg(percDone)
                                     .arg(progSpeed(), progTimeLeft()));
    }
}

void ProcState::toPercents(int bytes)
{
    if (totalSize_ == 0)
        return;

    if (doneSize_ == 0)
        emit percentageChanged(0); // initial 0 to reset progressbar value

    int lastPerc = (doneSize_ * 100) / totalSize_; // before current chunk added

    doneSize_ += bytes;
    int curPerc = (doneSize_ * 100) / totalSize_; // after

    if (curPerc > lastPerc)
        emit percentageChanged(curPerc);
}

QString ProcState::progTimeLeft()
{
    QString result;

    if (pieceSize > 0) {
        qint64 timeleft = ((totalSize_ - doneSize_) / pieceSize) * pieceTime;
        result = format::millisecToReadable(timeleft, true);
    }

    return result;
}

QString ProcState::progSpeed()
{
    QString result;

    if (pieceTime > 0) {
        result = QString("%1/sec")
                 .arg(format::dataSizeReadable((pieceSize / pieceTime) * 1000));
    }

    return result;
}

void ProcState::donePiece()
{
    pieceTime = elapsedTimer.elapsed() - prevTimePassed; // milliseconds
    pieceSize = doneSize_ - prevDoneSize;
    prevTimePassed += pieceTime;
    prevDoneSize += pieceSize;
}
