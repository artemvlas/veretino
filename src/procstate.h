/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef PROCSTATE_H
#define PROCSTATE_H

#include <QObject>
#include "nums.h"

class ProcState : public QObject
{
    Q_OBJECT

public:
    explicit ProcState(QObject *parent = nullptr);
    void setTotal(const NumSize &_nums);
    void setTotalSize(qint64 totalSize);
    void changeTotalSize(qint64 totalSize);
    bool decreaseTotalSize(qint64 by_size);

    // decreases the total number of elements in the queue [_p_queue] by number
    bool decreaseTotalQueued(int by_num = 1);

    // increases the number of finished pieces from the queue by one
    void addDoneOne();

    enum State {
        Idle = 1 << 0,
        StartSilently = 1 << 1,
        StartVerbose = 1 << 2,                  // set processing view (enable progress bar, change view model, etc...)
        Started = StartSilently | StartVerbose,
        Stop = 1 << 3,                          // stops the current operation, it is assumed that the list of done things will be saved
        Abort = 1 << 4,                         // interrupts the process, an immediate exit is expected (switching to the file system)
        Canceled = Stop | Abort
    };
    Q_ENUM(State)
    //Q_DECLARE_FLAGS(States, State)

    void setState(State state);
    State state() const; // returns current state_
    bool isState(State state) const;
    bool isStarted() const;
    bool isCanceled() const;
    bool hasTotalSize() const;

    // returns the total size of the processed data
    qint64 doneSize() const;

    // returns the size of the data processed since the previous function call
    qint64 donePieceSize() const;

    qint64 remainingSize() const;
    Chunks<qint64> pSize() const;
    Chunks<int> pQueue() const;

public slots:
    void addChunk(int chunk);

private:
    void startProgress();
    void isFinished();

    static qint64 prevDoneSize_;
    int lastPerc_ = 0; // percentage before current chunk added
    Chunks<qint64> _p_size;
    Chunks<int> _p_queue;

    State state_ = Idle;

signals:
    void stateChanged();
    void percentageChanged(int perc);
    void progressStarted();
    void progressFinished();
}; // class ProcState

using State = ProcState::State;
//Q_DECLARE_OPERATORS_FOR_FLAGS(ProcState::States)

#endif // PROCSTATE_H
