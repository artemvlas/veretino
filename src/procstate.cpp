/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "procstate.h"

qint64 ProcState::prevDoneSize_ = 0;

ProcState::ProcState(QObject *parent)
    : QObject{parent}
{}

void ProcState::setState(State state)
{
    if (state != state_) {
        state_ = state;
        emit stateChanged();
        isFinished();
    }
}

State ProcState::state() const
{
    return state_;
}

bool ProcState::isState(State state) const
{
    return (state_ & state);
}

bool ProcState::isStarted() const
{
    return (state_ & Started);
}

bool ProcState::isCanceled() const
{
    return (state_ & Canceled);
}

void ProcState::setTotalSize(qint64 totalSize)
{
    totalSize_ = totalSize;
    doneSize_ = 0;
}

void ProcState::startProgress()
{
    prevDoneSize_ = 0;
    lastPerc_ = 0;
    setState(StartVerbose);
    emit progressStarted();
}

void ProcState::isFinished()
{
    if (doneSize_ > 0 && totalSize_ > 0 && isState(Idle)) {
        setTotalSize(0); // reset to 0
        emit progressFinished();
    }
}

// add this processed piece(chunk), calculate total done size and emit ::percentageChanged
void ProcState::addChunk(int chunk)
{
    if (totalSize_ == 0)
        return;

    if (doneSize_ == 0)
        startProgress();

    doneSize_ += chunk;
    int curPerc = (doneSize_ * 100) / totalSize_; // current percentage

    if (curPerc > lastPerc_) {
        lastPerc_ = curPerc;
        emit percentageChanged(curPerc);
    }
}

qint64 ProcState::doneSize() const
{
    return doneSize_;
}

qint64 ProcState::donePieceSize() const
{
    qint64 pieceSize = doneSize_ - prevDoneSize_;
    prevDoneSize_ += pieceSize;

    return pieceSize;
}

qint64 ProcState::remainingSize() const
{
    return totalSize_ - doneSize_;
}
