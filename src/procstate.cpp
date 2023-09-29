#include "procstate.h"
#include "tools.h"
//#include <QDebug>

ProcState::ProcState(qint64 totalSize, QObject *parent)
    : QObject{parent}, totalSize_(totalSize)
{
    connect(this, &ProcState::donePercents, this, &ProcState::curStatus);
}

void ProcState::doneChunk(int chunk)
{
    toPercents(chunk);
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

    return estimatedTime;
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
