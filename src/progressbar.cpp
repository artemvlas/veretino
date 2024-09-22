/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "progressbar.h"
#include "tools.h"
#include <QStringBuilder>
#include <QDebug>

ProgressBar::ProgressBar(QWidget *parent)
    : QProgressBar(parent)
{
    setVisible(false);
    connect(timer, &QTimer::timeout, this, &ProgressBar::updateProgressInfo);
}

void ProgressBar::setProcState(const ProcState *proc)
{
    procState_ = proc;
}

void ProgressBar::start()
{
    setProgEnabled(true);
}

void ProgressBar::finish()
{
    setProgEnabled(false);
}

void ProgressBar::setProgEnabled(bool enabled)
{
    if (enabled) {
        resetFormat();
        timer->start(1000); // 1 sec
        elapsedTimer.start();
    }
    else {
        timer->stop();
    }

    setVisible(enabled);
    setValue(0);
}

void ProgressBar::updateProgressInfo()
{
    if (procState_ && procState_->isState(State::StartVerbose)) {
        updateDonePiece();

        static const QString _perc("%p%");
        static const QString _sep(" | ");
        QString _format = _perc % _sep % progSpeed() % _sep % progTimeLeft();
        setFormat(_format);

        // OLD
        // setFormat(QString("%p% | %1 | %2").arg(progSpeed(), progTimeLeft()));
    }
    else {
        finish();
        resetFormat();

        if (procState_)
            qDebug() << "<!> ProgressBar >> wrong state:" << procState_->state();
    }
}

void ProgressBar::updateDonePiece()
{
    pieceTime_ = elapsedTimer.restart();
    pieceSize_ = procState_->donePieceSize();
}

QString ProgressBar::progTimeLeft() const
{
    if (pieceSize_ > 0) {
        qint64 timeleft = (procState_->remainingSize() / pieceSize_) * pieceTime_;
        return format::millisecToReadable(timeleft, true);
    }

    return QStringLiteral(u"∞");
}

QString ProgressBar::progSpeed() const
{
    if (pieceTime_ > 0 && pieceSize_ > 0) {
        QString _s = format::dataSizeReadable((pieceSize_ / pieceTime_) * 1000);
        return _s + QStringLiteral(u"/s");
    }

    return QStringLiteral(u"idle");
}
