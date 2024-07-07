/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "statusbar.h"
#include <QDebug>
#include "tools.h"

StatusBar::StatusBar(QWidget *parent)
    : QStatusBar(parent)
{
    setStyleSheet("QWidget { margin: 0; }");
    statusTextLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::MinimumExpanding);
    statusIconLabel->setContentsMargins(5, 0, 0, 0);
    statusTextLabel->setContentsMargins(0, 0, 30, 0);
    addWidget(statusIconLabel);
    addWidget(statusTextLabel, 1);
}

void StatusBar::setIconProvider(const IconProvider *iconProvider)
{
    icons_ = iconProvider;
}

void StatusBar::setStatusText(const QString &text)
{
    statusTextLabel->setText(text);
}

void StatusBar::setStatusIcon(const QIcon &icon)
{
    statusIconLabel->setPixmap(icon.pixmap(16, 16));
}

void StatusBar::setModeFs(bool addButtonFilter)
{
    clearButtons();

    if (addButtonFilter) {
        if (!buttonFsFilter) {
            buttonFsFilter = addPermanentButton();
            buttonFsFilter->setIcon(icons_->icon(Icons::Filter));
            buttonFsFilter->setToolTip("View/Change Permanent Filter");
            connect(buttonFsFilter, &StatusBarButton::clicked, this, &StatusBar::buttonFsFilterClicked);
        }

        buttonFsFilter->show();
    }
}

void StatusBar::setModeDb(const DataContainer *data)
{
    clearButtons();

    // adding buttons if needed
    if (!buttonDbHash) {
        buttonDbHash = addPermanentButton();
        if (icons_)
            buttonDbHash->setIcon(icons_->icon(Icons::HashFile));
        connect(buttonDbHash, &StatusBarButton::clicked, this, &StatusBar::buttonDbHashClicked);
    }

    if (!buttonDbSize) {
        buttonDbSize = addPermanentButton();
        if (icons_)
            buttonDbSize->setIcon(icons_->icon(Icons::ChartPie));
        connect(buttonDbSize, &StatusBarButton::clicked, this, &StatusBar::buttonDbContentsClicked);
    }

    if (!buttonDbMain) {
        buttonDbMain = addPermanentButton();
        if (icons_)
            buttonDbMain->setIcon(icons_->icon(Icons::Database));
        connect(buttonDbMain, &StatusBarButton::clicked, this, &StatusBar::buttonDbListedClicked);
    }

    // update info
    const Numbers &numbers = data->numbers;

    QString checkResult = QString("☒ %1\n✓ %2")
                              .arg(numbers.numberOf(FileStatus::Mismatched))
                              .arg(numbers.numberOf(FileStatus::FlagMatched));

    buttonDbHash->setToolTip(checkResult);
    buttonDbHash->setText(format::algoToStr(data->metaData.algorithm));
    buttonDbSize->setText(format::dataSizeReadable(numbers.totalSize(FileStatus::FlagAvailable)));

    QString strDbMain = QString::number(numbers.numberOf(FileStatus::FlagAvailable));
    if (numbers.contains(FileStatus::Missing)) // if not all files are available, display "available/total"
        strDbMain.append(QString("/%1").arg(numbers.numberOf(FileStatus::FlagHasChecksum)));

    buttonDbMain->setText(strDbMain);

    buttonDbHash->show();
    buttonDbSize->show();
    buttonDbMain->show();
}

void StatusBar::setModeDbCreating()
{
    clearButtons();

    if (!buttonDbCreating) {
        buttonDbCreating = addPermanentButton();
        if (icons_)
            buttonDbCreating->setIcon(icons_->icon(Icons::Database));
        buttonDbCreating->setText("Creating...");
        connect(buttonDbCreating, &StatusBarButton::clicked, this, &StatusBar::buttonDbListedClicked);
    }

    buttonDbCreating->show();
}

StatusBarButton* StatusBar::addPermanentButton()
{
    StatusBarButton *button = new StatusBarButton(this);
    addPermanentWidget(button);

    return button;
}

void StatusBar::setButtonsEnabled(bool enable)
{
    QList<StatusBarButton*> list = findChildren<StatusBarButton*>();
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i)->isVisible())
            list.at(i)->setEnabled(enable);
    }
}

void StatusBar::clearButtons()
{
    QList<StatusBarButton*> list = findChildren<StatusBarButton*>();
    for (int i = 0; i < list.size(); ++i) {
        //removeWidget(list.at(i));
        list.at(i)->hide();
    }
}
