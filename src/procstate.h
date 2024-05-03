/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
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

    qint64 doneSize() const; // returns the total size of the processed data
    qint64 donePieceSize() const; // returns the size of the data processed since the previous function call
    qint64 remainingSize() const; // totalSize_ - doneSize_

public slots:
    void addChunk(int chunk);

private:
    void startProgress();
    void toPercents(int bytes); // add this processed piece, calculate total done size and emit ::percentageChanged

    static qint64 prevDoneSize_;
    qint64 totalSize_ = 0;
    qint64 doneSize_ = 0;

signals:
    void percentageChanged(int perc);
};

#endif // PROCSTATE_H
