/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef PROGRESSBAR_H
#define PROGRESSBAR_H

#include <QProgressBar>
#include <QTimer>
#include <QElapsedTimer>
#include "procstate.h"

class ProgressBar : public QProgressBar
{
    Q_OBJECT
public:
    explicit ProgressBar(QWidget *parent = nullptr);
    void setProcState(const ProcState *proc);

public slots:
    void start();
    void finish();

private:
    void setProgEnabled(bool enabled);
    void updateProgressInfo();
    void updateDonePiece();
    QString progTimeLeft();
    QString progSpeed();

    const ProcState *procState_ = nullptr;
    QTimer *timer = new QTimer(this);

    QElapsedTimer elapsedTimer;
    qint64 pieceTime_; // milliseconds
    qint64 pieceSize_;
}; // class ProgressBar

#endif // PROGRESSBAR_H
