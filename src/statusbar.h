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
#include <QLabel>
#include <QEvent>
#include "iconprovider.h"
#include "datacontainer.h"

class StatusBarButton : public QPushButton
{
    Q_OBJECT
public:
    StatusBarButton(QWidget *parent = nullptr)
        : QPushButton(parent)
    {
        setFlat(true);
        setFocusPolicy(Qt::NoFocus);
        setCursor(Qt::PointingHandCursor);
    }

protected:
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::ToolTip && !isEnabled())
            return true;

        return QPushButton::event(event);
    }
}; // class StatusBarButton

class StatusBar : public QStatusBar
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget *parent = nullptr);

    void setIconProvider(const IconProvider *iconProvider);
    void setStatusText(const QString &text);
    void setStatusIcon(const QIcon &icon);

    //void setModeFs(bool addButtonFilter);
    void setModeDb(const DataContainer *data);
    void setModeDbCreating();

    void clearButtons();
    void setButtonsEnabled(bool enable);
    void clear();

private:
    StatusBarButton* addPermanentButton();

    const IconProvider *icons_ = nullptr;
    QLabel *statusTextLabel = new QLabel(this);
    QLabel *statusIconLabel = new QLabel(this);
    StatusBarButton *buttonFsFilter = nullptr;
    StatusBarButton *buttonDbHash = nullptr;
    StatusBarButton *buttonDbSize = nullptr;
    StatusBarButton *buttonDbMain = nullptr;
    StatusBarButton *buttonDbCreating = nullptr;

signals:
    void buttonFsFilterClicked();
    void buttonDbListedClicked();
    void buttonDbContentsClicked();
    void buttonDbHashClicked();
}; // class StatusBar

#endif // STATUSBAR_H
