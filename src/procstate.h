/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef PROCSTATE_H
#define PROCSTATE_H

#include <QObject>

template <typename T = int> // (int, qint64...)
struct Pieces {
    Pieces() {}
    Pieces(T total) : _total(total) {}
    T remain() const { return _total - _done; }
    int percent() const { return (_done * 100) / _total; }
    void setTotal(T total) { _total = total; _done = 0; }
    bool decreaseTotal(T by_size) { if (remain() < by_size) return false; _total -= by_size; return true; }
    bool hasSet() const { return _total > 0; }
    bool hasChunks() const { return _done > 0; }
    void addChunks(T number) { _done += number; } // same as P <<
    void addChunk() { ++_done; } // same as ++P
    Pieces& operator<<(T done_num) { _done += done_num; return *this; }
    Pieces& operator++() { ++_done; return *this; } // prefix
    Pieces  operator++(int) { Pieces _res(*this); ++(*this); return _res; } // postfix
    explicit operator bool() const { return _total > 0; } // same as hasSet()

    // values
    T _total = 0;
    T _done = 0;
}; // struct Pieces

class ProcState : public QObject
{
    Q_OBJECT
public:
    explicit ProcState(QObject *parent = nullptr);
    void setTotalSize(qint64 totalSize);
    void changeTotalSize(qint64 totalSize);
    bool decreaseTotalSize(qint64 by_size);

    enum State {
        Idle = 1 << 0,
        StartSilently = 1 << 1,
        StartVerbose = 1 << 2, // set processing view (enable progress bar, change view model, etc...)
        Started = StartSilently | StartVerbose,
        Stop = 1 << 3, // stops the current operation, it is assumed that the list of done things will be saved
        Abort = 1 << 4, // interrupts the process, an immediate exit is expected (switching to the file system)
        Canceled = Stop | Abort
    };
    Q_ENUM(State)
    //Q_DECLARE_FLAGS(States, State)

    void setState(State state);
    State state() const; // returns current state_
    bool isState(State state) const;
    bool isStarted() const;
    bool isCanceled() const;

    qint64 doneSize() const; // returns the total size of the processed data
    qint64 donePieceSize() const; // returns the size of the data processed since the previous function call
    qint64 remainingSize() const;
    Pieces<qint64> piecesSize() const;

public slots:
    void addChunk(int chunk);

private:
    void startProgress();
    void isFinished();

    static qint64 prevDoneSize_;
    int lastPerc_ = 0; // percentage before current chunk added
    Pieces<qint64> _p_size;

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
