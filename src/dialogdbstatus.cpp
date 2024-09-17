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
        ui->labelWorkDir->setText("WorkDir: Specified");

    // datetime
    const QString (&dt)[3] = data_->metaData.datetime;

    if (dt[DateTimeStr::DateUpdated].isEmpty())
        ui->labelDateTime_Update->setText(dt[DateTimeStr::DateCreated]);
    else {
        ui->labelDateTime_Update->setText(dt[DateTimeStr::DateUpdated]);
        ui->labelDateTime_Update->setToolTip(dt[DateTimeStr::DateCreated]);
    }

    ui->labelDateTime_Check->setText(dt[DateTimeStr::DateVerified]);
}

void DialogDbStatus::setTabsInfo()
{
    IconProvider icons(palette()); // to set tabs icons

    // tab Content
    ui->labelContentNumbers->setText(infoContent().join('\n'));
    ui->tabWidget->setTabIcon(TabListed, icons.icon(Icons::Database));

    // tab Filter
    ui->tabWidget->setTabEnabled(TabFilter, data_->isFilterApplied());
    if (data_->isFilterApplied()) {
        ui->tabWidget->setTabIcon(TabFilter, icons.icon(Icons::Filter));

        ui->labelFiltersInfo->setStyleSheet(format::coloredText(data_->metaData.filter.isFilter(FilterRule::Ignore)));

        QString extensions = data_->metaData.filter.extensionString();
        data_->metaData.filter.isFilter(FilterRule::Include) ? ui->labelFiltersInfo->setText("Included:\n" + extensions)
                                                             : ui->labelFiltersInfo->setText("Ignored:\n" + extensions);
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
        return { "The checksum list is being calculated..." };

    const Numbers &_num = data_->numbers;

    QStringList contentNumbers;
    QString createdDataSize;
    const int numChecksums = _num.numberOf(FileStatus::CombHasChecksum);
    const int available = _num.numberOf(FileStatus::CombAvailable);
    const qint64 totalSize = _num.totalSize(FileStatus::CombAvailable);

    if (isJustCreated())
        createdDataSize = QString(" (%1)").arg(format::dataSizeReadable(totalSize));

    if (isJustCreated() || (numChecksums != available))
        contentNumbers.append(QString("Stored checksums: %1%2").arg(numChecksums).arg(createdDataSize));

    if (data_->contains(FileStatus::Unreadable))
        contentNumbers.append("Unreadable files: " + _num.numberOf(FileStatus::Unreadable));

    if (isSavedToDesktop()) {
        contentNumbers.append(QString());
        contentNumbers.append("Unable to save to working folder!");
        contentNumbers.append("The database is saved on the Desktop.");
    }

    if (isJustCreated())
        return contentNumbers;

    if (available > 0)
        contentNumbers.append(QStringLiteral(u"Available: ") + format::filesNumberAndSize(available, totalSize));
    else
        contentNumbers.append("NO FILES available to check");

    contentNumbers.append(QString());
    contentNumbers.append("***");

    //OLD: Files::itemInfo(data_->model_, FileStatus::New));
    contentNumbers.append(QStringLiteral(u"New:     ") + format::filesNumberAndSize(_num.numberOf(FileStatus::New), _num.totalSize(FileStatus::New)));
    contentNumbers.append(QStringLiteral(u"Missing: ") + format::filesNumber(_num.numberOf(FileStatus::Missing)));

    if (data_->isImmutable()) {
        contentNumbers.append(QString());
        contentNumbers.append("No changes are allowed");
    }

    return contentNumbers;
}

QStringList DialogDbStatus::infoVerification()
{
    QStringList result;
    const int available = data_->numbers.numberOf(FileStatus::CombAvailable);
    const int numChecksums = data_->numbers.numberOf(FileStatus::CombHasChecksum);

    if (data_->isAllChecked()) {
        if (data_->contains(FileStatus::Mismatched)) {
            result.append(QString("☒ %1 mismatches out of %2 available files")
                              .arg(data_->numbers.numberOf(FileStatus::Mismatched))
                              .arg(available));
        }
        else if (numChecksums == available)
            result.append(QString("✓ ALL %1 stored checksums matched").arg(numChecksums));
        else
            result.append(QString("✓ All %1 available files matched the stored checksums").arg(available));
    }
    else if (data_->contains(FileStatus::CombChecked)) {
        // to account for added and updated files, the total number in parentheses is used
        const int numAddedUpdated = data_->numbers.numberOf(FileStatus::Added | FileStatus::Updated);

        // info str
        const int numChecked = data_->numbers.numberOf(FileStatus::CombChecked);
        result.append(QString("%1%2 out of %3 files were checked")
                          .arg(numChecked)
                          .arg(numAddedUpdated > 0 ? QString("(%1)").arg(numChecked + numAddedUpdated) : QString())
                          .arg(available));

        result.append(QString());
        if (data_->contains(FileStatus::Mismatched))
            result.append(QString("%1 files MISMATCHED").arg(data_->numbers.numberOf(FileStatus::Mismatched)));
        else
            result.append("No Mismatches found");

        if (data_->contains(FileStatus::Matched)) {
            const int numMatched = data_->numbers.numberOf(FileStatus::Matched);
            result.append(QString("%1%2 files matched")
                              .arg(numMatched)
                              .arg(numAddedUpdated > 0 ? QString("(%1)").arg(numMatched + numAddedUpdated) : QString()));
        }
    }

    return result;
}

QStringList DialogDbStatus::infoChanges()
{
    const Numbers &_num = data_->numbers;
    QStringList result;

    if (data_->contains(FileStatus::Added)) // OLD: Files::itemInfo(data_->model_, FileStatus::Added)
        result.append(QStringLiteral(u"Added: ") + format::filesNumberAndSize(_num.numberOf(FileStatus::Added), _num.totalSize(FileStatus::Added)));

    if (data_->contains(FileStatus::Removed))
        result.append(QString("Removed: %1").arg(_num.numberOf(FileStatus::Removed)));

    if (data_->contains(FileStatus::Updated))
        result.append(QString("Updated: %1").arg(_num.numberOf(FileStatus::Updated)));

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
