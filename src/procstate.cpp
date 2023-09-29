#include "procstate.h"
//#include <QDebug>

ProcState::ProcState(qint64 totalSize, QObject *parent)
    : QObject{parent}, totalSize_(totalSize)
{}

void ProcState::doneChunk(int chunk)
{
    toPercents(chunk);
}

void ProcState::toPercents(int bytes)
{
    if (doneSize_ == 0) {
        emit donePercents(0); // initial 0 to reset progressbar value
        calcLeftTime(0);
    }

    int lastPerc = (doneSize_ * 100) / totalSize_; // before current chunk added

    doneSize_ += bytes;
    int curPerc = (doneSize_ * 100) / totalSize_; // after

    if (curPerc > lastPerc) {
        emit donePercents(curPerc);
        calcLeftTime(curPerc);
    }
}

void ProcState::calcLeftTime(const int percentsDone)
{
    if (percentsDone == 0) {
        elapsedTimer.start();
        return;
    }

    qint64 timePassed = elapsedTimer.elapsed();
    int leftPercents = 100 - percentsDone;

    qint64 timeleft = (timePassed / percentsDone) * leftPercents;
    int seconds = timeleft / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;
    int hours = minutes / 60;
    minutes = minutes % 60;

    QString estimatedTime;
    if (hours > 0)
        estimatedTime = QString("%1 h %2 min").arg(hours).arg(minutes);
    else if (minutes > 0 && seconds > 10)
        estimatedTime = QString("%1 min").arg(minutes + 1);
    else if (minutes > 0)
        estimatedTime = QString("%1 min").arg(minutes);
    else if (seconds > 4)
        estimatedTime = QString("%1 sec").arg(seconds);
    else
        estimatedTime = QString("few sec");

    //qDebug() << estimatedTime;
    emit timeLeft(estimatedTime);
}
