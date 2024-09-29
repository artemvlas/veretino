/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogdbstatus.h"
#include "ui_dialogdbstatus.h"
#include "clickablelabel.h"
#include <QFile>
#include <QDebug>
#include "iconprovider.h"
#include "tools.h"

DialogDbStatus::DialogDbStatus(const DataContainer *data, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogDbStatus)
    , data_(data)
{
    ui->setupUi(this);
    setWindowIcon(IconProvider::appIcon());

    if (data_->isDbFileState(DbFileState::NotSaved))
        setWindowTitle(windowTitle() + QStringLiteral(u" [unsaved]"));

    if (data_->isImmutable())
        setWindowTitle(windowTitle() + QStringLiteral(u" [const]"));

    setLabelsInfo();
    setTabsInfo();
    setVisibleTabs();
    connections();
}

DialogDbStatus::~DialogDbStatus()
{
    delete ui;
}

void DialogDbStatus::connections()
{
    connect(ui->labelDbFileName, &ClickableLabel::doubleClicked, this, [=]{ paths::browsePath(paths::parentFolder(data_->metaData.databaseFilePath)); });
    connect(ui->labelWorkDir, &ClickableLabel::doubleClicked, this, [=]{ paths::browsePath(data_->metaData.workDir); });
}

void DialogDbStatus::setLabelsInfo()
{
    QString dbFileName = data_->databaseFileName();

    if (isSavedToDesktop())
        dbFileName.prepend("../DESKTOP/");

    ui->labelDbFileName->setText(dbFileName);
    ui->labelDbFileName->setToolTip(data_->metaData.databaseFilePath);
    ui->labelAlgo->setText(QStringLiteral(u"Algorithm: ") + format::algoToStr(data_->metaData.algorithm));
    ui->labelWorkDir->setToolTip(data_->metaData.workDir);

    if (!data_->isWorkDirRelative())
        ui->labelWorkDir->setText(QStringLiteral(u"WorkDir: Specified"));

    // datetime
    const QString (&dt)[3] = data_->metaData.datetime;

    if (dt[DTstr::DateUpdated].isEmpty())
        ui->labelDateTime_Update->setText(dt[DTstr::DateCreated]);
    else {
        ui->labelDateTime_Update->setText(dt[DTstr::DateUpdated]);
        ui->labelDateTime_Update->setToolTip(dt[DTstr::DateCreated]);
    }

    ui->labelDateTime_Check->setText(dt[DTstr::DateVerified]);
}

void DialogDbStatus::setTabsInfo()
{
    IconProvider icons(palette()); // to set tabs icons

    // tab Content
    ui->labelContentNumbers->setStyleSheet(QStringLiteral(u"QLabel { font-family: monospace; }"));
    ui->labelContentNumbers->setText(infoContent().join('\n'));
    ui->tabWidget->setTabIcon(TabListed, icons.icon(Icons::Database));

    // tab Filter
    ui->tabWidget->setTabEnabled(TabFilter, data_->isFilterApplied());
    if (data_->isFilterApplied()) {
        ui->tabWidget->setTabIcon(TabFilter, icons.icon(Icons::Filter));

        const FilterRule &_filter = data_->metaData.filter;
        ui->labelFiltersInfo->setStyleSheet(format::coloredText(_filter.isFilter(FilterRule::Ignore)));

        const QString _exts = _filter.extensionString();
        const QString _str_mode = _filter.isFilter(FilterRule::Include) ? QStringLiteral(u"Included:\n")
                                                                        : QStringLiteral(u"Ignored:\n");
        ui->labelFiltersInfo->setText(_str_mode + _exts);
    }

    // tab Verification
    ui->tabWidget->setTabEnabled(TabVerification, data_->contains(FileStatus::CombChecked));
    if (data_->contains(FileStatus::CombChecked)) {
        ui->tabWidget->setTabIcon(TabVerification, icons.icon(Icons::DoubleGear));
        ui->labelVerification->setText(infoVerification().join('\n'));
    }

    // tab Result
    ui->tabWidget->setTabEnabled(TabChanges, !isJustCreated() && data_->contains(FileStatus::CombDbChanged));
    if (ui->tabWidget->isTabEnabled(TabChanges)) {
        ui->tabWidget->setTabIcon(TabChanges, icons.icon(Icons::Update));
        ui->labelResult->setText(infoChanges().join('\n'));
    }
}

QStringList DialogDbStatus::infoContent()
{
    if (isCreating())
        return { QStringLiteral(u"The checksum list is being calculated...") };

    const Numbers &_num = data_->numbers;
    QStringList contentNumbers;
    const qint64 totalSize = _num.totalSize(FileStatus::CombAvailable);
    const int available = _num.numberOf(FileStatus::CombAvailable);
    const int numChecksums = _num.numberOf(FileStatus::CombHasChecksum);

    if (isJustCreated() || (numChecksums != available)) {
        QString _storedChecksums = tools::joinStrings(QStringLiteral(u"Stored checksums:"), numChecksums);

        if (isJustCreated())
            contentNumbers << format::addStrInParentheses(_storedChecksums, format::dataSizeReadable(totalSize));
        else
            contentNumbers << _storedChecksums;
    }

    if (data_->contains(FileStatus::CombUnreadable))
        contentNumbers.append(tools::joinStrings(QStringLiteral(u"Unreadable files:"), _num.numberOf(FileStatus::CombUnreadable)));

    if (isSavedToDesktop()) {
        contentNumbers.append(QString());
        contentNumbers.append("Unable to save to working folder!");
        contentNumbers.append("The database is saved on the Desktop.");
    }

    if (isJustCreated())
        return contentNumbers;

    if (available > 0)
        contentNumbers.append(QStringLiteral(u"Available: ") + format::filesNumSize(available, totalSize));
    else
        contentNumbers.append("NO FILES available to check");

    // [experimental]
    if (_num.contains(FileStatus::NotCheckedMod))
        contentNumbers << tools::joinStrings(QStringLiteral(u"Modified: "), _num.numberOf(FileStatus::NotCheckedMod));
    // [exp.]

    contentNumbers.append(QString());
    contentNumbers.append(QStringLiteral(u"***"));

    contentNumbers.append(QStringLiteral(u"New:     ") + format::filesNumSize(_num, FileStatus::New));
    contentNumbers.append(QStringLiteral(u"Missing: ") + format::filesNumber(_num.numberOf(FileStatus::Missing)));

    if (data_->isImmutable()) {
        contentNumbers.append(QString());
        contentNumbers.append(QStringLiteral(u"No changes are allowed"));
    }

    return contentNumbers;
}

QStringList DialogDbStatus::infoVerification()
{
    QStringList result;
    const Numbers &num = data_->numbers;
    const int available = num.numberOf(FileStatus::CombAvailable);
    const int numChecksums = num.numberOf(FileStatus::CombHasChecksum);

    if (data_->isAllChecked()) {
        if (data_->contains(FileStatus::Mismatched)) {
            result.append(QString("☒ %1 mismatches out of %2 available files")
                              .arg(num.numberOf(FileStatus::Mismatched))
                              .arg(available));
        }
        else if (numChecksums == available)
            result.append(QString("✓ ALL %1 stored checksums matched").arg(numChecksums));
        else
            result.append(QString("✓ All %1 available files matched the stored checksums").arg(available));
    }
    else if (data_->contains(FileStatus::CombChecked)) {
        // to account for added and updated files, the total number in parentheses is used
        const int numAddedUpdated = num.numberOf(FileStatus::Added | FileStatus::Updated);

        // info str
        const int numChecked = num.numberOf(FileStatus::CombChecked);
        result.append(QString("%1%2 out of %3 files were checked")
                          .arg(numChecked)
                          .arg(numAddedUpdated > 0 ? format::inParentheses(numChecked + numAddedUpdated) : QString())
                          .arg(available));

        result.append(QString());
        if (data_->contains(FileStatus::Mismatched))
            result.append(format::filesNumber(num.numberOf(FileStatus::Mismatched)) + QStringLiteral(u" MISMATCHED"));
        else
            result.append(QStringLiteral(u"No Mismatches found"));

        if (data_->contains(FileStatus::Matched)) {
            const int numMatched = num.numberOf(FileStatus::Matched);
            result.append(QString("%1%2 files matched")
                              .arg(numMatched)
                              .arg(numAddedUpdated > 0 ? format::inParentheses(numMatched + numAddedUpdated) : QString()));
        }
    }

    return result;
}

QStringList DialogDbStatus::infoChanges()
{
    const Numbers &_num = data_->numbers;
    QStringList result;

    if (data_->contains(FileStatus::Added))
        result.append(QStringLiteral(u"Added: ") + format::filesNumSize(_num, FileStatus::Added));

    if (data_->contains(FileStatus::Removed))
        result.append(tools::joinStrings(QStringLiteral(u"Removed:"), _num.numberOf(FileStatus::Removed)));

    if (data_->contains(FileStatus::Updated))
        result.append(tools::joinStrings(QStringLiteral(u"Updated:"), _num.numberOf(FileStatus::Updated)));

    return result;
}

void DialogDbStatus::setVisibleTabs()
{
    // hide disabled tabs
    for (int var = 0; var < ui->tabWidget->count(); ++var) {
        ui->tabWidget->setTabVisible(var, ui->tabWidget->isTabEnabled(var));
    }
}
void DialogDbStatus::selectCurTab()
{
    // selecting the tab to open
    Tabs curTab = TabListed;
    if (ui->tabWidget->isTabEnabled(TabChanges))
        curTab = TabChanges;
    else if (ui->tabWidget->isTabEnabled(TabVerification))
        curTab = TabVerification;

    ui->tabWidget->setCurrentIndex(curTab);
}

void DialogDbStatus::setCurrentTab(Tabs tab)
{
    if (tab != TabAutoSelect && ui->tabWidget->isTabEnabled(tab)) {
        ui->tabWidget->setCurrentIndex(tab);
        autoTabSelection = false;
    }
}

bool DialogDbStatus::isCreating()
{
    return data_->isDbFileState(MetaData::NoFile);
}

bool DialogDbStatus::isJustCreated()
{
    return data_->isDbFileState(MetaData::Created);
}

bool DialogDbStatus::isSavedToDesktop()
{
    return (isJustCreated() && !data_->isWorkDirRelative()
            && (paths::parentFolder(data_->metaData.databaseFilePath) == Files::desktopFolderPath));
}

void DialogDbStatus::showEvent(QShowEvent *event)
{
    if (autoTabSelection)
        selectCurTab();

    QDialog::showEvent(event);
}
