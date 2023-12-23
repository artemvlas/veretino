#include "dbstatusdialog.h"
#include "ui_dbstatusdialog.h"

DbStatusDialog::DbStatusDialog(const DataContainer *data, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DbStatusDialog)
{
    ui->setupUi(this);

    //setFixedSize(400,300);
    setWindowIcon(QIcon(":/veretino.png"));

    ui->labelDbFileName->setText(data->databaseFileName());
    ui->labelDbFileName->setToolTip(data->metaData.databaseFilePath);
    ui->labelAlgo->setText(QString("Algorithm: %1").arg(format::algoToStr(data->metaData.algorithm)));
    ui->labelWorkDir->setToolTip(data->metaData.workDir);

    if (!data->isWorkDirRelative())
        ui->labelWorkDir->setText("WorkDir: Predefined");

    ui->labelDateTime_Update->setText("Updated: " + data->metaData.saveDateTime);
    ui->labelDateTime_Check->clear(); // not yet implemented

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

    // selecting the tab to open
    Tabs curTab = data->containsChecked() ? TabVerification : TabContent;
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

    if (data->numbers.numberOf(FileStatus::Unreadable) > 0)
        contentNumbers.append(QString("Unreadable files: %1").arg(data->numbers.numberOf(FileStatus::Unreadable)));

    contentNumbers.append(QString());
    contentNumbers.append("***");

    if (data->numbers.numberOf(FileStatus::New) > 0)
        contentNumbers.append("New: " + Files::itemInfo(data->model_, {FileStatus::New}));
    else
        contentNumbers.append("No New files found");

    if (data->numbers.numberOf(FileStatus::Missing) > 0)
        contentNumbers.append(QString("Missing: %1 files").arg(data->numbers.numberOf(FileStatus::Missing)));
    else
        contentNumbers.append("No Missing files found");

    if (data->numbers.numberOf(FileStatus::New) > 0 || data->numbers.numberOf(FileStatus::Missing) > 0) {
        contentNumbers.append(QString());
        contentNumbers.append("Use a context menu for more options");
    }

    return contentNumbers;
}

QStringList DbStatusDialog::infoVerification(const DataContainer *data)
{
    QStringList result;

    if (data->isAllChecked()) {
        if (data->numbers.numberOf(FileStatus::Mismatched) > 0)
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
        if (data->numbers.numberOf(FileStatus::Mismatched) > 0)
            result.append(QString("%1 files MISMATCHED").arg(data->numbers.numberOf(FileStatus::Mismatched)));
        else
            result.append("No Mismatches found");

        if (data->numbers.numberOf(FileStatus::Matched) > 0)
            result.append(QString("%1 files matched").arg(data->numbers.numberOf(FileStatus::Matched)));
    }

    return result;
}

DbStatusDialog::~DbStatusDialog()
{
    delete ui;
}
