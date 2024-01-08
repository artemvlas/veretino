/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "foldercontentsdialog.h"
#include "ui_foldercontentsdialog.h"
#include "tools.h"

FolderContentsDialog::FolderContentsDialog(const QString &folderPath, const QList<ExtNumSize> &extList, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FolderContentsDialog)
    , extList_(extList)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/veretino.png"));
    ui->treeWidget->setColumnWidth(ColumnExtension, 130);
    ui->treeWidget->setColumnWidth(ColumnFilesNumber, 130);
    ui->treeWidget->sortByColumn(ColumnTotalSize, Qt::DescendingOrder);

    QString prefix = (paths::parentFolder(folderPath) == "/") ? "/" : ".../";
    ui->labelFolderName->setText(prefix + paths::basicName(folderPath));
    ui->labelFolderName->setToolTip(folderPath);
    ui->checkBox_Top10->setVisible(extList.size() > 15);

    connect(ui->checkBox_Top10, &QCheckBox::toggled, this, &FolderContentsDialog::populate);

    setTotalInfo();
    populate();
}

FolderContentsDialog::~FolderContentsDialog()
{
    delete ui;
}

void FolderContentsDialog::addItemToTreeWidget(const ExtNumSize &itemData)
{
    TreeWidgetItem *item = new TreeWidgetItem(ui->treeWidget);
    item->setData(ColumnExtension, Qt::DisplayRole, itemData.extension);
    item->setData(ColumnFilesNumber, Qt::DisplayRole, itemData.filesNumber);
    item->setData(ColumnTotalSize, Qt::DisplayRole, format::dataSizeReadable(itemData.filesSize));
    item->setData(ColumnTotalSize, Qt::UserRole, itemData.filesSize);
    ui->treeWidget->addTopLevelItem(item);
}

void FolderContentsDialog::populate()
{
    ui->treeWidget->clear();

    if (ui->checkBox_Top10->isVisible() && ui->checkBox_Top10->isChecked()) {
        QList<ExtNumSize> list = extList_;
        if (ui->treeWidget->sortColumn() == ColumnFilesNumber)
            std::sort(list.begin(), list.end(), [](const ExtNumSize &t1, const ExtNumSize &t2) {return (t1.filesNumber > t2.filesNumber);});
        else
            std::sort(list.begin(), list.end(), [](const ExtNumSize &t1, const ExtNumSize &t2) {return (t1.filesSize > t2.filesSize);});

        int top10FilesNumber = 0; // total number of files in the Top10 list
        qint64 top10FilesSize = 0; // total size of these files

        for (int i = 0; i < 10; ++i) {
            addItemToTreeWidget(list.at(i));
            top10FilesNumber += list.at(i).filesNumber;
            top10FilesSize += list.at(i).filesSize;
        }

        ui->checkBox_Top10->setText(QString("Top10: %1 files (%2)")
                                            .arg(top10FilesNumber)
                                            .arg(format::dataSizeReadable(top10FilesSize)));
        return;
    }

    ui->checkBox_Top10->setText("Top10");

    for (int i = 0; i < extList_.size(); ++i) {
        addItemToTreeWidget(extList_.at(i));
    }
}

void FolderContentsDialog::setTotalInfo()
{
    qint64 totalSize = 0;
    int totalFilesNumber = 0;

    for (int i = 0; i < extList_.size(); ++i) {
        totalSize += extList_.at(i).filesSize;
        totalFilesNumber += extList_.at(i).filesNumber;
    }

    ui->labelTotal->setText(QString("Total: %1 types, %2 files (%3) ")
                                .arg(extList_.size())
                                .arg(totalFilesNumber)
                                .arg(format::dataSizeReadable(totalSize)));
}
