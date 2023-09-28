#include "procstate.h"

ProcState::ProcState(qint64 totalSize, QObject *parent)
    : QObject{parent}, totalSize_(totalSize)
{}

void ProcState::doneChunk(int chunk)
{
    toPercents(chunk);
}

void ProcState::toPercents(int bytes)
{
    if (doneSize_ == 0)
        emit donePercents(0); // initial 0 to reset progressbar value

    int lastPerc = (doneSize_ * 100) / totalSize_; // before current chunk added

    doneSize_ += bytes;
    int curPerc = (doneSize_ * 100) / totalSize_; // after

    if (curPerc > lastPerc) {
        emit donePercents(curPerc);
    }
}
