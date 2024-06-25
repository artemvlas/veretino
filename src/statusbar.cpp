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
    //setStyleSheet("QWidget { margin: 0; }");
    statusTextLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::MinimumExpanding);
    statusIconLabel->setContentsMargins(5, 0, 0, 0);
    statusTextLabel->setContentsMargins(0, 0, 30, 0);
    addWidget(statusIconLabel);
    addWidget(statusTextLabel, 1);
    addPermanentWidget(permanentStatus);
}

StatusBar::~StatusBar()
{
    qDebug() << Q_FUNC_INFO;
}

void StatusBar::setStatusText(const QString &text)
{
    statusTextLabel->setText(text);
}

void StatusBar::setStatusIcon(const QIcon &icon)
{
    statusIconLabel->setPixmap(icon.pixmap(16, 16));
}

void StatusBar::clearButtons()
{
    QList<QPushButton*> list = findChildren<QPushButton*>();
    for (int i = 0; i < list.size(); ++i) {
        removeWidget(list.at(i));
    }
}

QPushButton* StatusBar::createButton()
{
    QPushButton *button = new QPushButton(this);
    button->setFlat(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setCursor(Qt::PointingHandCursor);

    return button;
}
