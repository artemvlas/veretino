/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef PROCSTATE_H
#define PROCSTATE_H

#include <QObject>
#include <QElapsedTimer>

class ProcState : public QObject
{
    Q_OBJECT
public:
    explicit ProcState(qint64 totalSize, QObject *parent = nullptr);
    qint64 doneSize();

public slots:
    void doneChunk(int chunk);

private:
    void start();
    void curStatus(int percDone);
    void toPercents(int bytes); // add this processed piece, calculate total done size and emit donePercents()
    QString calcLeftTime(const int percentsDone);
    QString calcSpeed(int percDone);
    void donePiece();

    QElapsedTimer elapsedTimer;
    qint64 prevTimePassed;
    qint64 prevDoneSize;
    qint64 pieceTime;
    qint64 pieceSize;

    qint64 totalSize_ = 0; // total data size
    qint64 doneSize_ = 0;

signals:
    void donePercents(int perc);
    void procStatus(const QString &str);
};

#endif // PROCSTATE_H
