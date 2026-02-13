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
    m_statusTextLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::MinimumExpanding);
    m_statusIconLabel->setContentsMargins(5, 0, 0, 0);
    m_statusTextLabel->setContentsMargins(0, 0, 30, 0);
    addWidget(m_statusIconLabel);
    addWidget(m_statusTextLabel, 1);
}

void StatusBar::setIconProvider(const IconProvider *iconProvider)
{
    m_icons = iconProvider;
}

void StatusBar::setStatusText(const QString &text)
{
    m_statusTextLabel->setText(text);
}

void StatusBar::setStatusIcon(const QIcon &icon)
{
    m_statusIconLabel->setPixmap(icon.pixmap(16, 16));
}

void StatusBar::setModeDb(const DataContainer *data)
{
    clearButtons();

    // adding buttons if needed
    if (!m_buttonDbHash) {
        m_buttonDbHash = addPermanentButton();
        if (m_icons)
            m_buttonDbHash->setIcon(m_icons->icon(Icons::HashFile));
        connect(m_buttonDbHash, &StatusBarButton::clicked, this, &StatusBar::buttonDbHashClicked);
    }

    if (!m_buttonDbSize) {
        m_buttonDbSize = addPermanentButton();
        if (m_icons)
            m_buttonDbSize->setIcon(m_icons->icon(Icons::ChartPie));
        connect(m_buttonDbSize, &StatusBarButton::clicked, this, &StatusBar::buttonDbContentsClicked);
    }

    if (!m_buttonDbMain) {
        m_buttonDbMain = addPermanentButton();
        if (m_icons)
            m_buttonDbMain->setIcon(m_icons->icon(Icons::Database));
        connect(m_buttonDbMain, &StatusBarButton::clicked, this, &StatusBar::buttonDbListedClicked);
    }

    // update info
    const Numbers &num = data->m_numbers;

    QString checkResult = QString("☒ %1\n✓ %2")
                              .arg(num.numberOf(FileStatus::Mismatched))
                              .arg(num.numberOf(FileStatus::CombMatched));

    m_buttonDbHash->setToolTip(checkResult);
    m_buttonDbHash->setText(format::algoToStr(data->m_metadata.algorithm));
    m_buttonDbSize->setText(format::dataSizeReadable(num.totalSize(FileStatus::CombAvailable)));

    QString strDbMain = QString::number(num.numberOf(FileStatus::CombAvailable));
    if (num.contains(FileStatus::Missing)) // if not all files are available, display "available/total"
        strDbMain.append(QString("/%1").arg(num.numberOf(FileStatus::CombHasChecksum)));

    m_buttonDbMain->setText(strDbMain);

    m_buttonDbHash->show();
    m_buttonDbSize->show();
    m_buttonDbMain->show();
}

void StatusBar::setModeDbCreating()
{
    clearButtons();

    if (!m_buttonDbCreating) {
        m_buttonDbCreating = addPermanentButton();
        if (m_icons)
            m_buttonDbCreating->setIcon(m_icons->icon(Icons::Database));
        m_buttonDbCreating->setText(QStringLiteral(u"Creating..."));
        connect(m_buttonDbCreating, &StatusBarButton::clicked, this, &StatusBar::buttonDbListedClicked);
    }

    m_buttonDbCreating->show();
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

    for (StatusBarButton *btn : buttons) {
        if (btn->isVisible())
            btn->setEnabled(enable);
    }
}

void StatusBar::clearButtons()
{
    const QList<StatusBarButton*> buttons = findChildren<StatusBarButton*>();

    for (StatusBarButton *btn : buttons) {
        btn->hide();
    }
}

void StatusBar::clear()
{
    clearButtons();
    m_statusIconLabel->clear();
    m_statusTextLabel->clear();
}
