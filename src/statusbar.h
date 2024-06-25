/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>
#include <QIcon>
#include <QPushButton>
#include "clickablelabel.h"
#include "iconprovider.h"

class StatusBar : public QStatusBar
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget *parent = nullptr);
    ~StatusBar();

    void setIconProvider(const IconProvider *iconProvider);
    void setStatusText(const QString &text);
    void setStatusIcon(const QIcon &icon);

    ClickableLabel *permanentStatus = new ClickableLabel(this);

private:
    void clearButtons();
    QPushButton* createButton();

    const IconProvider *icons_ = nullptr;
    QLabel *statusTextLabel = new QLabel(this);
    QLabel *statusIconLabel = new QLabel(this);
}; // class StatusBar

#endif // STATUSBAR_H
