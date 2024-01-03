// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "dbstatusdialog.h"
#include "ui_dbstatusdialog.h"
#include <QFile>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

DbStatusDialog::DbStatusDialog(const DataContainer *data, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DbStatusDialog)
    , data_(data)
{
    ui->setupUi(this);

    //setFixedSize(400,300);
    setWindowIcon(QIcon(":/veretino.png"));

    connect(ui->labelDbFileName, &ClickableLabel::doubleClicked, this, [=]{browsePath(paths::parentFolder(data_->metaData.databaseFilePath));});
    connect(ui->labelWorkDir, &ClickableLabel::doubleClicked, this, &DbStatusDialog::browseWorkDir);

    ui->labelDbFileName->setText(data->databaseFileName());
    ui->labelDbFileName->setToolTip(data->metaData.databaseFilePath);
    ui->labelAlgo->setText(QString("Algorithm: %1").arg(format::algoToStr(data->metaData.algorithm)));
    ui->labelWorkDir->setToolTip(data->metaData.workDir);

    if (!data->isWorkDirRelative())
        ui->labelWorkDir->setText("WorkDir: Predefined");

    ui->labelDateTime_Update->setText("Updated: " + data->metaData.saveDateTime);
    data->metaData.successfulCheckDateTime.isEmpty() ? ui->labelDateTime_Check->clear()
                                                     : ui->labelDateTime_Check->setText("Verified: " + data->metaData.successfulCheckDateTime);

    // tab Content
    ui->labelContentNumbers->setText(infoContent(data).join("\n"));

    // tab Filter
    ui->tabWidget->setTabVisible(TabFilters, data->isFilterApplied());
    if (data->isFilterApplied()) {
        QString extensions = data->metaData.filter.extensionsList.join(", ");
        data->metaData.filter.isFilter(FilterRule::Include) ? ui->labelFiltersInfo->setText(QString("Included Only:\n%1").arg(extensions))
                                                            : ui->labelFiltersInfo->setText(QString("Ignored:\n%1").arg(extensions));
    }

    // tab Verification
    ui->tabWidget->setTabVisible(TabVerification, data->containsChecked());
    if (data->containsChecked()) {
        ui->labelVerification->setText(infoVerification(data).join("\n"));
    }

    // tab Result
    ui->tabWidget->setTabVisible(TabChanges, data->contains({FileStatus::Added, FileStatus::Removed, FileStatus::ChecksumUpdated}));
    if (ui->tabWidget->isTabVisible(TabChanges)) {
        ui->labelResult->setText(infoChanges().join("\n"));
    }

    // selecting the tab to open
    Tabs curTab = TabContent;
    if (ui->tabWidget->isTabVisible(TabChanges))
        curTab = TabChanges;
    else if (ui->tabWidget->isTabVisible(TabVerification))
        curTab = TabVerification;

    ui->tabWidget->setCurrentIndex(curTab);
}

QStringList DbStatusDialog::infoContent(const DataContainer *data)
{
    QStringList contentNumbers;

    if (data->numbers.numChecksums != data->numbers.available())
        contentNumbers.append(QString("Stored checksums: %1").arg(data->numbers.numChecksums));

    if (data->numbers.available() > 0)
        contentNumbers.append(QString("Available: %1").arg(format::filesNumberAndSize(data->numbers.available(), data->numbers.totalSize)));
    else
        contentNumbers.append("NO FILES available to check");

    if (data->contains(FileStatus::Unreadable))
        contentNumbers.append(QString("Unreadable files: %1").arg(data->numbers.numberOf(FileStatus::Unreadable)));

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

    if (data->contains({FileStatus::New, FileStatus::Missing})) {
        contentNumbers.append(QString());
        contentNumbers.append("Use a context menu for more options");
    }

    return contentNumbers;
}

QStringList DbStatusDialog::infoVerification(const DataContainer *data)
{
    QStringList result;

    if (data->isAllChecked()) {
        if (data->contains(FileStatus::Mismatched))
            result.append(QString("☒ %1 mismatches of %2 checksums")
                              .arg(data->numbers.numberOf(FileStatus::Mismatched))
                              .arg(data->numbers.numChecksums));

        else if (data->numbers.numChecksums == data->numbers.available())
            result.append(QString("✓ ALL %1 stored checksums matched").arg(data->numbers.numChecksums));
        else
            result.append(QString("✓ All %1 available files matched the stored checksums").arg(data->numbers.available()));
    }
    else if (data->containsChecked()) {
        result.append(QString("%1 out of %2 files were checked")
                          .arg(data->numbers.numberOf(FileStatus::Matched) + data->numbers.numberOf(FileStatus::Mismatched))
                          .arg(data->numbers.available()));

        result.append(QString());
        //result.append("Current check result:");
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

    if (data_->contains(FileStatus::ChecksumUpdated))
        result.append(QString("Updated: %1").arg(data_->numbers.numberOf(FileStatus::ChecksumUpdated)));

    return result;
}

void DbStatusDialog::browsePath(const QString &path)
{
    if (QFile::exists(path)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void DbStatusDialog::browseWorkDir()
{
    browsePath(data_->metaData.workDir);
}

DbStatusDialog::~DbStatusDialog()
{
    delete ui;
}
