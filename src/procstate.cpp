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
    //totalSize_ = totalSize;
    //doneSize_ = 0;
    _p_size.setTotal(totalSize);
}

void ProcState::changeTotalSize(qint64 totalSize)
{
    /*if (totalSize > doneSize_) {
        totalSize_ = totalSize;
    }*/
    if (totalSize > _p_size._done) {
        _p_size._total = totalSize;
    }
}

bool ProcState::decreaseTotalSize(qint64 by_size)
{
    return _p_size.decreaseTotal(by_size);
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
    if (_p_size.hasChunks() && _p_size.hasSet() && isState(Idle)) {
        setTotalSize(0); // reset to 0
        emit progressFinished();
    }
}

// add this processed piece(chunk), calculate total done size and emit ::percentageChanged
void ProcState::addChunk(int chunk)
{
    if (!_p_size.hasSet()) // _p_size._total == 0
        return;

    if (!_p_size.hasChunks()) // _p_size._done == 0
        startProgress();

    //doneSize_ += chunk;
    _p_size << chunk;
    int curPerc = _p_size.percent(); //(doneSize_ * 100) / totalSize_; // current percentage

    if (curPerc > lastPerc_) {
        lastPerc_ = curPerc;
        emit percentageChanged(curPerc);
    }
}

qint64 ProcState::doneSize() const
{
    //return doneSize_;
    return _p_size._done;
}

qint64 ProcState::donePieceSize() const
{
    qint64 pieceSize = _p_size._done - prevDoneSize_;
    prevDoneSize_ += pieceSize;

    return pieceSize;
}

qint64 ProcState::remainingSize() const
{
    //return totalSize_ - doneSize_;
    return _p_size.remain();
}
