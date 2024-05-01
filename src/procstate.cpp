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
    connect(this, &ProcState::donePercents, this, &ProcState::curStatus);
}

void ProcState::doneChunk(int chunk)
{
    toPercents(chunk);
}

qint64 ProcState::doneSize()
{
    return doneSize_;
}

void ProcState::start()
{
    prevTimePassed = 0;
    prevDoneSize = 0;
    elapsedTimer.start();
}

void ProcState::curStatus(int percDone)
{
    if (percDone == 0)
        start();
    else {
        donePiece();
        emit procStatus(QString("%1% | %2 | %3").arg(percDone).arg(calcSpeed(percDone), calcLeftTime(percDone)));
    }
}

void ProcState::toPercents(int bytes)
{
    if (totalSize_ == 0)
        return;

    if (doneSize_ == 0)
        emit donePercents(0); // initial 0 to reset progressbar value

    int lastPerc = (doneSize_ * 100) / totalSize_; // before current chunk added

    doneSize_ += bytes;
    int curPerc = (doneSize_ * 100) / totalSize_; // after

    if (curPerc > lastPerc)
        emit donePercents(curPerc);
}

QString ProcState::calcLeftTime(const int percentsDone)
{
    if (percentsDone == 0) {
        start();
        return QString();
    }

    //qint64 timePassed = elapsedTimer.elapsed();
    //int leftPercents = 100 - percentsDone;
    //qint64 timeleft = (timePassed / percentsDone) * leftPercents;

    qint64 timeleft = ((totalSize_ - doneSize_) / pieceSize) * pieceTime;

    return format::millisecToReadable(timeleft, true);
}

QString ProcState::calcSpeed(int percDone)
{
    QString result;

    if (percDone == 0) {
        start();
    }
    else {
        if (pieceTime > 0)
            result = QString("%1/sec").arg(format::dataSizeReadable((pieceSize / pieceTime) * 1000)); // bytes per second
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
