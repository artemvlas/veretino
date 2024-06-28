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
#include "iconprovider.h"
#include "datacontainer.h"

class StatusBar : public QStatusBar
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget *parent = nullptr);
    ~StatusBar();

    void setIconProvider(const IconProvider *iconProvider);
    void setStatusText(const QString &text);
    void setStatusIcon(const QIcon &icon);

    void setModeFs(bool addButtonFilter);
    void setModeDb(const DataContainer *data);
    void setModeDbCreating();

    void clearButtons();
    void setButtonsEnabled(bool enable);

private:
    QPushButton* createButton();
    QPushButton* addPermanentButton();

    const IconProvider *icons_ = nullptr;
    QLabel *statusTextLabel = new QLabel(this);
    QLabel *statusIconLabel = new QLabel(this);
    QPushButton *buttonFsFilter = nullptr;
    QPushButton *buttonDbHash = nullptr;
    QPushButton *buttonDbSize = nullptr;
    QPushButton *buttonDbMain = nullptr;
    QPushButton *buttonDbCreating = nullptr;

signals:
    void buttonFsFilterClicked();
    void buttonDbStatusClicked();
    void buttonDbContentsClicked();
}; // class StatusBar

#endif // STATUSBAR_H
