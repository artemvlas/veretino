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
    addPermanentWidget(permanentStatus);
}

StatusBar::~StatusBar()
{
    qDebug() << Q_FUNC_INFO;
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
    permanentStatus->clear();

    if (addButtonFilter) {
        if (!buttonFsFilter) {
            buttonFsFilter = createButton();
            buttonFsFilter->setIcon(icons_->icon(Icons::Filter));
            buttonFsFilter->setToolTip("View/Change Permanent Filter");
            connect(buttonFsFilter, &QPushButton::clicked, this, &StatusBar::buttonFsFilterClicked);
        }
        addPermanentWidget(buttonFsFilter);
        buttonFsFilter->show();
    }
}

void StatusBar::setModeDb(const QString &permStatus)
{
    clearButtons();
    permanentStatus->setText(permStatus);
}

void StatusBar::setModeDb(const DataContainer *data)
{
    clearButtons();
    permanentStatus->clear();

    if (!buttonDbHash) {
        buttonDbHash = createButton();
        if (icons_)
            buttonDbHash->setIcon(icons_->icon(Icons::HashFile));
        connect(buttonDbHash, &QPushButton::clicked, this, &StatusBar::buttonFsFilterClicked);
    }

    if (!buttonDbSize) {
        buttonDbSize = createButton();
        if (icons_)
            buttonDbSize->setIcon(icons_->icon(Icons::ChartPie));
        connect(buttonDbSize, &QPushButton::clicked, this, &StatusBar::buttonFsFilterClicked);
    }

    if (!buttonDbMain) {
        buttonDbMain = createButton();
        if (icons_)
            buttonDbMain->setIcon(icons_->icon(Icons::Database));
        connect(buttonDbMain, &QPushButton::clicked, this, &StatusBar::buttonFsFilterClicked);
    }

    const Numbers &numbers = data->numbers;

    buttonDbHash->setText(format::algoToStr(data->metaData.algorithm));
    buttonDbSize->setText(format::dataSizeReadable(numbers.totalSize(FileStatus::FlagAvailable)));
    buttonDbMain->setText(QString::number(numbers.numberOf(FileStatus::FlagAvailable)));

    addPermanentWidget(buttonDbHash);
    addPermanentWidget(buttonDbSize);
    addPermanentWidget(buttonDbMain);
    buttonDbHash->show();
    buttonDbSize->show();
    buttonDbMain->show();
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
