/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "statusbar.h"
#include <QDebug>

StatusBar::StatusBar(QWidget *parent)
    : QStatusBar(parent)
{
    statusTextLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::MinimumExpanding);
    statusIconLabel->setContentsMargins(5, 0, 0, 0);
    permanentStatus->setContentsMargins(20, 0, 0, 0);
    addWidget(statusIconLabel);
    addWidget(statusTextLabel, 1);
    addPermanentWidget(permanentStatus);
}

StatusBar::~StatusBar()
{
    qDebug() << Q_FUNC_INFO;
}
