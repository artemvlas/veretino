#include "foldercontentsdialog.h"
#include "ui_foldercontentsdialog.h"
#include "tools.h"

FolderContentsDialog::FolderContentsDialog(const QString &folderName, const QList<ExtNumSize> &extList, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::FolderContentsDialog)
    , extList_(extList)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/veretino.png"));
    ui->treeWidget->setColumnWidth(ColumnExtension, 130);
    ui->treeWidget->setColumnWidth(ColumnFilesNumber, 130);
    ui->treeWidget->sortByColumn(ColumnTotalSize, Qt::DescendingOrder);

    ui->labelFolderName->setText(".../" + folderName);
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

        qint64 excSize = 0; // total size of files whose types are not displayed
        int excNumber = 0; // the number of these files
        for (int i = 0; i < list.size(); ++i) {
            if (i < 10)
                addItemToTreeWidget(list.at(i));
            else {
                excSize += list.at(i).filesSize;
                excNumber += list.at(i).filesNumber;
            }
        }

        ui->checkBox_Top10->setText(QString("Top10, Excluded: %1 types, %2").arg(list.size() - 10).arg(format::filesNumberAndSize(excNumber, excSize)));
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

    ui->labelTotal->setText(QString("Total: %1 types, %2 files (%3)  ")
                                .arg(extList_.size())
                                .arg(totalFilesNumber)
                                .arg(format::dataSizeReadable(totalSize)));
}
