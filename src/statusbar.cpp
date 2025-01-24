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
    setStyleSheet(QStringLiteral(u"QWidget { margin: 0; }"));
    statusTextLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::MinimumExpanding);
    statusIconLabel->setContentsMargins(5, 0, 0, 0);
    statusTextLabel->setContentsMargins(0, 0, 30, 0);
    addWidget(statusIconLabel);
    addWidget(statusTextLabel, 1);
}

void StatusBar::setIconProvider(const IconProvider *iconProvider)
{
    p_icons = iconProvider;
}

void StatusBar::setStatusText(const QString &text)
{
    statusTextLabel->setText(text);
}

void StatusBar::setStatusIcon(const QIcon &icon)
{
    statusIconLabel->setPixmap(icon.pixmap(16, 16));
}

void StatusBar::setModeDb(const DataContainer *data)
{
    clearButtons();

    // adding buttons if needed
    if (!buttonDbHash) {
        buttonDbHash = addPermanentButton();
        if (p_icons)
            buttonDbHash->setIcon(p_icons->icon(Icons::HashFile));
        connect(buttonDbHash, &StatusBarButton::clicked, this, &StatusBar::buttonDbHashClicked);
    }

    if (!buttonDbSize) {
        buttonDbSize = addPermanentButton();
        if (p_icons)
            buttonDbSize->setIcon(p_icons->icon(Icons::ChartPie));
        connect(buttonDbSize, &StatusBarButton::clicked, this, &StatusBar::buttonDbContentsClicked);
    }

    if (!buttonDbMain) {
        buttonDbMain = addPermanentButton();
        if (p_icons)
            buttonDbMain->setIcon(p_icons->icon(Icons::Database));
        connect(buttonDbMain, &StatusBarButton::clicked, this, &StatusBar::buttonDbListedClicked);
    }

    // update info
    const Numbers &_num = data->m_numbers;

    QString checkResult = QString("☒ %1\n✓ %2")
                              .arg(_num.numberOf(FileStatus::Mismatched))
                              .arg(_num.numberOf(FileStatus::CombMatched));

    buttonDbHash->setToolTip(checkResult);
    buttonDbHash->setText(format::algoToStr(data->metaData_.algorithm));
    buttonDbSize->setText(format::dataSizeReadable(_num.totalSize(FileStatus::CombAvailable)));

    QString strDbMain = QString::number(_num.numberOf(FileStatus::CombAvailable));
    if (_num.contains(FileStatus::Missing)) // if not all files are available, display "available/total"
        strDbMain.append(QString("/%1").arg(_num.numberOf(FileStatus::CombHasChecksum)));

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
        if (p_icons)
            buttonDbCreating->setIcon(p_icons->icon(Icons::Database));
        buttonDbCreating->setText(QStringLiteral(u"Creating..."));
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
    const QList<StatusBarButton*> buttons = findChildren<StatusBarButton*>();

    for (StatusBarButton *_button : buttons) {
        if (_button->isVisible())
            _button->setEnabled(enable);
    }
}

void StatusBar::clearButtons()
{
    const QList<StatusBarButton*> buttons = findChildren<StatusBarButton*>();

    for (StatusBarButton *_button : buttons) {
        _button->hide();
    }
}

void StatusBar::clear()
{
    clearButtons();
    statusIconLabel->clear();
    statusTextLabel->clear();
}
