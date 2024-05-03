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
    explicit ProcState(QObject *parent = nullptr);
    void setTotalSize(qint64 totalSize);
    void updateDonePiece();

    qint64 doneSize() const;
    QString progTimeLeft() const;
    QString progSpeed() const;

public slots:
    void addChunk(int chunk);

private:
    void startProgress();
    void toPercents(int bytes); // add this processed piece, calculate total done size and emit ::percentageChanged

    QElapsedTimer elapsedTimer;
    qint64 prevDoneSize;
    qint64 pieceTime; // milliseconds
    qint64 pieceSize;

    qint64 totalSize_ = 0; // total data size
    qint64 doneSize_ = 0;

signals:
    void percentageChanged(int perc);
};

#endif // PROCSTATE_H
