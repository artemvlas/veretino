/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "dbstatusdialog.h"
#include "ui_dbstatusdialog.h"
#include "clickablelabel.h"
#include <QFile>
#include <QDebug>
#include "iconprovider.h"

DbStatusDialog::DbStatusDialog(const DataContainer *data, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DbStatusDialog)
    , data_(data)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/veretino.png"));

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

        QString extensions = data->metaData.filter.extensionsList.join(", ");
        data->metaData.filter.isFilter(FilterRule::Include) ? ui->labelFiltersInfo->setText(QString("Included Only:\n%1").arg(extensions))
                                                            : ui->labelFiltersInfo->setText(QString("Ignored:\n%1").arg(extensions));
    }

    // tab Verification
    ui->tabWidget->setTabEnabled(TabVerification, data->contains(FileStatusFlag::FlagChecked));
    if (data->contains(FileStatusFlag::FlagChecked)) {
        ui->tabWidget->setTabIcon(TabVerification, icons.icon(Icons::DoubleGear));
        ui->labelVerification->setText(infoVerification(data).join("\n"));
    }

    // tab Result
    ui->tabWidget->setTabEnabled(TabChanges, !isJustCreated()
                                             && data->contains({FileStatus::Added, FileStatus::Removed, FileStatus::Updated}));
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

QStringList DbStatusDialog::infoContent(const DataContainer *data)
{
    QStringList contentNumbers;
    QString createdDataSize;
    int available = data->numbers.numberOf(FileStatusFlag::FlagAvailable);

    if (isJustCreated())
        createdDataSize = QString(" (%1)").arg(format::dataSizeReadable(data->numbers.totalSize));

    if (isJustCreated() || (data->numbers.numChecksums != available))
        contentNumbers.append(QString("Stored checksums: %1%2").arg(data->numbers.numChecksums).arg(createdDataSize));

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
        contentNumbers.append(QString("Available: %1").arg(format::filesNumberAndSize(available, data->numbers.totalSize)));
    else
        contentNumbers.append("NO FILES available to check");

    contentNumbers.append(QString());
    contentNumbers.append("***");

    if (data->contains(FileStatus::New))
        contentNumbers.append("New: " + Files::itemInfo(data->model_, {FileStatus::New}));
    else
        contentNumbers.append("No New files found");

    if (data->contains(FileStatus::Missing))
        contentNumbers.append(QString("Missing: %1 files").arg(data->numbers.numberOf(FileStatus::Missing)));
    else
        contentNumbers.append("No Missing files found");

    if (data->contains(FileStatusFlag::FlagNewLost)) {
        contentNumbers.append(QString());
        contentNumbers.append("Use a context menu for more options");
    }

    return contentNumbers;
}

QStringList DbStatusDialog::infoVerification(const DataContainer *data)
{
    QStringList result;
    const int available = data->numbers.numberOf(FileStatusFlag::FlagAvailable);

    if (data->isAllChecked()) {
        if (data->contains(FileStatus::Mismatched))
            result.append(QString("☒ %1 mismatches out of %2 available files")
                              .arg(data->numbers.numberOf(FileStatus::Mismatched))
                              .arg(available));

        else if (data->numbers.numChecksums == available)
            result.append(QString("✓ ALL %1 stored checksums matched").arg(data->numbers.numChecksums));
        else
            result.append(QString("✓ All %1 available files matched the stored checksums").arg(available));
    }
    else if (data->contains(FileStatusFlag::FlagChecked)) {
        result.append(QString("%1 out of %2 files were checked")
                          .arg(data->numbers.numberOf(FileStatusFlag::FlagChecked))
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

QStringList DbStatusDialog::infoChanges()
{
    QStringList result;

    if (data_->contains(FileStatus::Added))
        result.append(QString("Added: %1").arg(Files::itemInfo(data_->model_, {FileStatus::Added})));

    if (data_->contains(FileStatus::Removed))
        result.append(QString("Removed: %1").arg(data_->numbers.numberOf(FileStatus::Removed)));

    if (data_->contains(FileStatus::Updated))
        result.append(QString("Updated: %1").arg(data_->numbers.numberOf(FileStatus::Updated)));

    return result;
}

bool DbStatusDialog::isJustCreated()
{
    return (!data_->metaData.isImported
            && data_->numbers.numberOf({FileStatus::Added, FileStatus::Matched, FileStatus::Mismatched}) == data_->numbers.numChecksums);
}

DbStatusDialog::~DbStatusDialog()
{
    delete ui;
}
