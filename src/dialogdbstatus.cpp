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

DialogDbStatus::DialogDbStatus(const DataContainer *data, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogDbStatus)
    , data_(data)
{
    ui->setupUi(this);
    setWindowIcon(IconProvider::appIcon());

    if (data_->isDbFileState(DbFileState::NotSaved))
        setWindowTitle(windowTitle() + " [unsaved]");

    setLabelsInfo();
    setTabsInfo();
    setVisibleTabs();
    connections();
}

void DialogDbStatus::connections()
{
    connect(ui->labelDbFileName, &ClickableLabel::doubleClicked, this, [&]{ paths::browsePath(paths::parentFolder(data_->metaData.databaseFilePath)); });
    connect(ui->labelWorkDir, &ClickableLabel::doubleClicked, this, [&]{ paths::browsePath(data_->metaData.workDir); });
}

void DialogDbStatus::setLabelsInfo()
{
    QString dbFileName = data_->databaseFileName();

    if (isSavedToDesktop())
        dbFileName.prepend(".../DESKTOP/");

    ui->labelDbFileName->setText(dbFileName);
    ui->labelDbFileName->setToolTip(data_->metaData.databaseFilePath);
    ui->labelAlgo->setText(QString("Algorithm: %1").arg(format::algoToStr(data_->metaData.algorithm)));
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
    ui->labelContentNumbers->setText(infoContent().join("\n"));
    ui->tabWidget->setTabIcon(TabListed, icons.icon(Icons::Database));

    // tab Filter
    ui->tabWidget->setTabEnabled(TabFilter, data_->isFilterApplied());
    if (data_->isFilterApplied()) {
        ui->tabWidget->setTabIcon(TabFilter, icons.icon(Icons::Filter));

        ui->labelFiltersInfo->setStyleSheet(format::coloredText(data_->metaData.filter.isFilter(FilterRule::Ignore)));

        QString extensions = data_->metaData.filter.extensionsList.join(", ");
        data_->metaData.filter.isFilter(FilterRule::Include) ? ui->labelFiltersInfo->setText(QString("Included:\n%1").arg(extensions))
                                                             : ui->labelFiltersInfo->setText(QString("Ignored:\n%1").arg(extensions));
    }

    // tab Verification
    ui->tabWidget->setTabEnabled(TabVerification, data_->contains(FileStatus::FlagChecked));
    if (data_->contains(FileStatus::FlagChecked)) {
        ui->tabWidget->setTabIcon(TabVerification, icons.icon(Icons::DoubleGear));
        ui->labelVerification->setText(infoVerification().join("\n"));
    }

    // tab Result
    ui->tabWidget->setTabEnabled(TabChanges, !isJustCreated() && data_->contains(FileStatus::FlagDbChanged));
    if (ui->tabWidget->isTabEnabled(TabChanges)) {
        ui->tabWidget->setTabIcon(TabChanges, icons.icon(Icons::Update));
        ui->labelResult->setText(infoChanges().join("\n"));
    }
}

void DialogDbStatus::setVisibleTabs()
{
    // hide disabled tabs
    for (int var = 0; var < ui->tabWidget->count(); ++var) {
        ui->tabWidget->setTabVisible(var, ui->tabWidget->isTabEnabled(var));
    }

    // selecting the tab to open
    Tabs curTab = TabListed;
    if (ui->tabWidget->isTabEnabled(TabChanges))
        curTab = TabChanges;
    else if (ui->tabWidget->isTabEnabled(TabVerification))
        curTab = TabVerification;

    ui->tabWidget->setCurrentIndex(curTab);
}

QStringList DialogDbStatus::infoContent()
{
    QStringList contentNumbers;
    QString createdDataSize;
    int numChecksums = data_->numbers.numberOf(FileStatus::FlagHasChecksum);
    int available = data_->numbers.numberOf(FileStatus::FlagAvailable);
    qint64 totalSize = data_->numbers.totalSize(FileStatus::FlagAvailable);

    if (isJustCreated())
        createdDataSize = QString(" (%1)").arg(format::dataSizeReadable(totalSize));

    if (isJustCreated() || (numChecksums != available))
        contentNumbers.append(QString("Stored checksums: %1%2").arg(numChecksums).arg(createdDataSize));

    if (data_->contains(FileStatus::Unreadable))
        contentNumbers.append(QString("Unreadable files: %1").arg(data_->numbers.numberOf(FileStatus::Unreadable)));

    if (isSavedToDesktop()) {
        contentNumbers.append(QString());
        contentNumbers.append("Unable to save to working folder!");
        contentNumbers.append("The database is saved in the Desktop folder.");
    }

    if (isJustCreated())
        return contentNumbers;

    if (available > 0)
        contentNumbers.append(QString("Available: %1").arg(format::filesNumberAndSize(available, totalSize)));
    else
        contentNumbers.append("NO FILES available to check");

    contentNumbers.append(QString());
    contentNumbers.append("***");

    if (data_->contains(FileStatus::New))
        contentNumbers.append("New: " + Files::itemInfo(data_->model_, FileStatus::New));
    else
        contentNumbers.append("No New files found");

    if (data_->contains(FileStatus::Missing))
        contentNumbers.append(QString("Missing: %1 files").arg(data_->numbers.numberOf(FileStatus::Missing)));
    else
        contentNumbers.append("No Missing files found");

    if (data_->contains(FileStatus::FlagNewLost)) {
        contentNumbers.append(QString());
        contentNumbers.append("Use a context menu for more options");
    }

    return contentNumbers;
}

QStringList DialogDbStatus::infoVerification()
{
    QStringList result;
    const int available = data_->numbers.numberOf(FileStatus::FlagAvailable);
    const int numChecksums = data_->numbers.numberOf(FileStatus::FlagHasChecksum);

    if (data_->isAllChecked()) {
        if (data_->contains(FileStatus::Mismatched))
            result.append(QString("☒ %1 mismatches out of %2 available files")
                              .arg(data_->numbers.numberOf(FileStatus::Mismatched))
                              .arg(available));

        else if (numChecksums == available)
            result.append(QString("✓ ALL %1 stored checksums matched").arg(numChecksums));
        else
            result.append(QString("✓ All %1 available files matched the stored checksums").arg(available));
    }
    else if (data_->contains(FileStatus::FlagChecked)) {
        // to account for added and updated files, the total number in parentheses is used
        int numAddedUpdated = data_->numbers.numberOf(FileStatus::Added | FileStatus::Updated);

        // info str
        int numChecked = data_->numbers.numberOf(FileStatus::FlagChecked);
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
            int numMatched = data_->numbers.numberOf(FileStatus::Matched);
            result.append(QString("%1%2 files matched")
                              .arg(numMatched)
                              .arg(numAddedUpdated > 0 ? QString("(%1)").arg(numMatched + numAddedUpdated) : QString()));
        }
    }

    return result;
}

QStringList DialogDbStatus::infoChanges()
{
    QStringList result;

    if (data_->contains(FileStatus::Added))
        result.append(QString("Added: %1").arg(Files::itemInfo(data_->model_, FileStatus::Added)));

    if (data_->contains(FileStatus::Removed))
        result.append(QString("Removed: %1").arg(data_->numbers.numberOf(FileStatus::Removed)));

    if (data_->contains(FileStatus::Updated))
        result.append(QString("Updated: %1").arg(data_->numbers.numberOf(FileStatus::Updated)));

    return result;
}

bool DialogDbStatus::isJustCreated()
{
    return (data_->metaData.dbFileState == MetaData::Created);
}

bool DialogDbStatus::isSavedToDesktop()
{
    return (isJustCreated() && !data_->isWorkDirRelative()
            && (paths::parentFolder(data_->metaData.databaseFilePath) == Files::desktopFolderPath));
}

DialogDbStatus::~DialogDbStatus()
{
    delete ui;
}
