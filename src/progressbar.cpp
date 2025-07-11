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
    connect(m_timer, &QTimer::timeout, this, &ProgressBar::updateProgressInfo);
}

void ProgressBar::setProcState(const ProcState *proc)
{
    m_proc = proc;
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
        m_timer->start(1000); // 1 sec
        m_elapsedTimer.start();
    } else {
        m_timer->stop();
    }

    setVisible(enabled);
    setValue(0);
}

void ProgressBar::updateProgressInfo()
{
    if (m_proc && m_proc->isState(State::StartVerbose)) {
        updateDonePiece();

        QString format = QStringLiteral(u"%p%")
                         % Lit::s_sepStick // " | "
                         % progSpeed()
                         % Lit::s_sepStick
                         % progTimeLeft();

        setFormat(format);
    } else {
        finish();
        resetFormat();

        if (m_proc)
            qDebug() << "<!> ProgressBar >> wrong state:" << m_proc->state();
    }
}

void ProgressBar::updateDonePiece()
{
    pieceTime_ = m_elapsedTimer.restart();
    pieceSize_ = m_proc->donePieceSize();
}

QString ProgressBar::progTimeLeft() const
{
    if (pieceSize_ > 0) {
        qint64 timeleft = (m_proc->remainingSize() / pieceSize_) * pieceTime_;
        return format::millisecToReadable(timeleft, true);
    }

    return QStringLiteral(u"∞");
}

QString ProgressBar::progSpeed() const
{
    if (pieceTime_ > 0 && pieceSize_ > 0) {
        QString str = format::dataSizeReadable((pieceSize_ / pieceTime_) * 1000);
        return str + QStringLiteral(u"/s");
    }

    return QStringLiteral(u"idle");
}
