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
    QString progTimeLeft() const;
    QString progSpeed() const;

    const ProcState *m_proc = nullptr;
    QTimer *m_timer = new QTimer(this);
    QElapsedTimer m_elapsedTimer;

    qint64 m_pieceTime; // milliseconds
    qint64 m_pieceSize;
}; // class ProgressBar

#endif // PROGRESSBAR_H
