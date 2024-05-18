/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
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
    emit progressStarted();
}

void ProcState::addChunk(int chunk)
{
    if (doneSize_ == 0)
        startProgress();

    toPercents(chunk);
}

void ProcState::toPercents(int bytes)
{
    if (totalSize_ == 0)
        return;

    int lastPerc = (doneSize_ * 100) / totalSize_; // before current chunk added

    doneSize_ += bytes;
    int curPerc = (doneSize_ * 100) / totalSize_; // after

    if (curPerc > lastPerc) {
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
