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
    if (state_ == state)
        return;

    state_ = state;
    emit stateChanged();
    isFinished();
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

void ProcState::setAwaiting(Awaiting await)
{
    awaiting_ = await;
}

bool ProcState::isAwaiting() const
{
    return awaiting_;
}

bool ProcState::isAwaiting(Awaiting await) const
{
    return awaiting_ == await;
}

bool ProcState::hasTotalSize() const
{
    return chunks_size_.isSet();
}

void ProcState::setTotal(const NumSize &nums)
{
    chunks_size_.setTotal(nums.total_size);
    chunks_queue_.setTotal(nums.number);
}

void ProcState::setTotalSize(qint64 totalSize)
{
    chunks_size_.setTotal(totalSize);
    chunks_queue_.reset();
}

void ProcState::changeTotalSize(qint64 totalSize)
{
    if (totalSize > chunks_size_.done) {
        chunks_size_.total = totalSize;
    }
}

bool ProcState::decreaseTotalSize(qint64 by_size)
{
    return chunks_size_.decreaseTotal(by_size);
}

bool ProcState::decreaseTotalQueued(int by_num)
{
    return chunks_queue_.decreaseTotal(by_num);
}

void ProcState::addDoneOne()
{
    ++chunks_queue_;
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
    if (chunks_size_.hasChunks() && chunks_size_.isSet() && isState(Idle)) {
        setTotalSize(0); // reset to 0
        emit progressFinished();
    }
}

// add this processed piece(chunk), calculate total done size and emit ::percentageChanged
void ProcState::addChunk(int chunk)
{
    if (!chunks_size_.isSet()) // chunks_size_.total == 0
        return;

    if (!chunks_size_.hasChunks()) // chunks_size_.done == 0
        startProgress();

    chunks_size_ << chunk;
    const int curPerc = chunks_size_.percent(); // current percentage

    if (curPerc > lastPerc_) {
        lastPerc_ = curPerc;
        emit percentageChanged(curPerc);
    }
}

qint64 ProcState::doneSize() const
{
    return chunks_size_.done;
}

qint64 ProcState::donePieceSize() const
{
    qint64 pieceSize = chunks_size_.done - prevDoneSize_;
    prevDoneSize_ += pieceSize;

    return pieceSize;
}

qint64 ProcState::remainingSize() const
{
    return chunks_size_.remain();
}

Chunks<qint64> ProcState::chunksSize() const
{
    return chunks_size_;
}

Chunks<int> ProcState::chunksQueue() const
{
    return chunks_queue_;
}
