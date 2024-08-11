/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef PROCSTATE_H
#define PROCSTATE_H

#include <QObject>

class ProcState : public QObject
{
    Q_OBJECT
public:
    explicit ProcState(QObject *parent = nullptr);
    void setTotalSize(qint64 totalSize);
    void changeTotalSize(qint64 totalSize);

    enum State {
        Idle = 1 << 0,
        StartSilently = 1 << 1,
        StartVerbose = 1 << 2, // set processing view (enable progress bar, change view model, etc...)
        Started = StartSilently | StartVerbose,
        Stop = 1 << 3, // stops the current task only
        Abort = 1 << 4, // terminates the current task and clears the queue
        Canceled = Stop | Abort
    };
    Q_ENUM(State)
    Q_DECLARE_FLAGS(States, State)

    void setState(State state);
    State state() const; // returns current state_
    bool isState(State state) const;
    bool isStarted() const;
    bool isCanceled() const;

    qint64 doneSize() const; // returns the total size of the processed data
    qint64 donePieceSize() const; // returns the size of the data processed since the previous function call
    qint64 remainingSize() const; // totalSize_ - doneSize_

public slots:
    void addChunk(int chunk);

private:
    void startProgress();
    void isFinished();

    static qint64 prevDoneSize_;
    int lastPerc_ = 0; // percentage before current chunk added
    qint64 totalSize_ = 0;
    qint64 doneSize_ = 0;

    State state_ = Idle;

signals:
    void stateChanged();
    void percentageChanged(int perc);
    void progressStarted();
    void progressFinished();
}; // class ProcState

using State = ProcState::State;
Q_DECLARE_OPERATORS_FOR_FLAGS(ProcState::States)

#endif // PROCSTATE_H
