/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
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

    setWindowIcon(IconProvider::iconVeretino());

    connect(ui->labelDbFileName, &ClickableLabel::doubleClicked, this, [=]{paths::browsePath(paths::parentFolder(data_->metaData.databaseFilePath));});
    connect(ui->labelWorkDir, &ClickableLabel::doubleClicked, this, [=]{paths::browsePath(data_->metaData.workDir);});

    QString dbFileName = data->databaseFileName();
    if (data->metaData.saveResult == MetaData::SavedToDesktop)
        dbFileName.prepend(".../DESKTOP/");

    ui->labelDbFileName->setText(dbFileName);
    ui->labelDbFileName->setToolTip(data->metaData.databaseFilePath);
    ui->labelAlgo->setText(QString("Algorithm: %1").arg(format::algoToStr(data->metaData.algorithm)));
    ui->labelWorkDir->setToolTip(data->metaData.workDir);

    if (!data->isWorkDirRelative())
        ui->labelWorkDir->setText("WorkDir: Specified");

    ui->labelDateTime_Update->setText("Updated: " + data->metaData.saveDateTime);
    data->metaData.successfulCheckDateTime.isEmpty() ? ui->labelDateTime_Check->clear()
                                                     : ui->labelDateTime_Check->setText("Verified: " + data->metaData.successfulCheckDateTime);

    IconProvider icons(palette()); // to set tabs icons

    // tab Content
    ui->labelContentNumbers->setText(infoContent(data).join("\n"));
    ui->tabWidget->setTabIcon(TabListed, icons.icon(Icons::Database));

    // tab Filter
    ui->tabWidget->setTabEnabled(TabFilter, data->isFilterApplied());
    if (data->isFilterApplied()) {
        ui->tabWidget->setTabIcon(TabFilter, icons.icon(Icons::Filter));

        ui->labelFiltersInfo->setStyleSheet(format::coloredText(data->metaData.filter.isFilter(FilterRule::Ignore)));

        QString extensions = data->metaData.filter.extensionsList.join(", ");
        data->metaData.filter.isFilter(FilterRule::Include) ? ui->labelFiltersInfo->setText(QString("Included:\n%1").arg(extensions))
                                                            : ui->labelFiltersInfo->setText(QString("Ignored:\n%1").arg(extensions));
    }

    // tab Verification
    ui->tabWidget->setTabEnabled(TabVerification, data->contains(FileStatus::FlagChecked));
    if (data->contains(FileStatus::FlagChecked)) {
        ui->tabWidget->setTabIcon(TabVerification, icons.icon(Icons::DoubleGear));
        ui->labelVerification->setText(infoVerification(data).join("\n"));
    }

    // tab Result
    ui->tabWidget->setTabEnabled(TabChanges, !isJustCreated() && data->contains(FileStatus::FlagDbChanged));
    if (ui->tabWidget->isTabEnabled(TabChanges)) {
        ui->tabWidget->setTabIcon(TabChanges, icons.icon(Icons::Update));
        ui->labelResult->setText(infoChanges().join("\n"));
    }

    // selecting the tab to open
    Tabs curTab = TabListed;
    if (ui->tabWidget->isTabEnabled(TabChanges))
        curTab = TabChanges;
    else if (ui->tabWidget->isTabEnabled(TabVerification))
        curTab = TabVerification;

    ui->tabWidget->setCurrentIndex(curTab);

    // hide disabled tabs
    for (int var = 0; var < ui->tabWidget->count(); ++var) {
        ui->tabWidget->setTabVisible(var, ui->tabWidget->isTabEnabled(var));
    }
}

QStringList DialogDbStatus::infoContent(const DataContainer *data)
{
    QStringList contentNumbers;
    QString createdDataSize;
    int numChecksums = data->numbers.numberOf(FileStatus::FlagHasChecksum);
    int available = data->numbers.numberOf(FileStatus::FlagAvailable);
    qint64 totalSize = data->numbers.totalSize(FileStatus::FlagAvailable);

    if (isJustCreated())
        createdDataSize = QString(" (%1)").arg(format::dataSizeReadable(totalSize));

    if (isJustCreated() || (numChecksums != available))
        contentNumbers.append(QString("Stored checksums: %1%2").arg(numChecksums).arg(createdDataSize));

    if (data->contains(FileStatus::Unreadable))
        contentNumbers.append(QString("Unreadable files: %1").arg(data->numbers.numberOf(FileStatus::Unreadable)));

    if (data->metaData.saveResult == MetaData::SavedToDesktop) {
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

    if (data->contains(FileStatus::New))
        contentNumbers.append("New: " + Files::itemInfo(data->model_, FileStatus::New));
    else
        contentNumbers.append("No New files found");

    if (data->contains(FileStatus::Missing))
        contentNumbers.append(QString("Missing: %1 files").arg(data->numbers.numberOf(FileStatus::Missing)));
    else
        contentNumbers.append("No Missing files found");

    if (data->contains(FileStatus::FlagNewLost)) {
        contentNumbers.append(QString());
        contentNumbers.append("Use a context menu for more options");
    }

    return contentNumbers;
}

QStringList DialogDbStatus::infoVerification(const DataContainer *data)
{
    QStringList result;
    const int available = data->numbers.numberOf(FileStatus::FlagAvailable);
    const int numChecksums = data->numbers.numberOf(FileStatus::FlagHasChecksum);

    if (data->isAllChecked()) {
        if (data->contains(FileStatus::Mismatched))
            result.append(QString("☒ %1 mismatches out of %2 available files")
                              .arg(data->numbers.numberOf(FileStatus::Mismatched))
                              .arg(available));

        else if (numChecksums == available)
            result.append(QString("✓ ALL %1 stored checksums matched").arg(numChecksums));
        else
            result.append(QString("✓ All %1 available files matched the stored checksums").arg(available));
    }
    else if (data->contains(FileStatus::FlagChecked)) {
        result.append(QString("%1 out of %2 files were checked")
                          .arg(data->numbers.numberOf(FileStatus::FlagChecked))
                          .arg(available));

        result.append(QString());
        if (data->contains(FileStatus::Mismatched))
            result.append(QString("%1 files MISMATCHED").arg(data->numbers.numberOf(FileStatus::Mismatched)));
        else
            result.append("No Mismatches found");

        if (data->contains(FileStatus::Matched))
            result.append(QString("%1 files matched").arg(data->numbers.numberOf(FileStatus::Matched)));
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
    FileStatuses flagJustCreated = (FileStatus::Added | FileStatus::FlagChecked);

    return (!data_->metaData.isImported
            && (data_->numbers.numberOf(flagJustCreated) == data_->numbers.numberOf(FileStatus::FlagHasChecksum)));
}

DialogDbStatus::~DialogDbStatus()
{
    delete ui;
}
