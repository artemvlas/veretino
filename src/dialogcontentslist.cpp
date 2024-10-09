/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogcontentslist.h"
#include "ui_dialogcontentslist.h"
#include "tools.h"
#include <QPushButton>
#include <QDebug>

DialogContentsList::DialogContentsList(const QString &folderPath, const FileTypeList &extList, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogContentsList)
{
    ui->setupUi(this);
    ui->types_->setColumnWidth(ItemFileType::ColumnType, 130);
    ui->types_->setColumnWidth(ItemFileType::ColumnFilesNumber, 130);
    ui->types_->sortByColumn(ItemFileType::ColumnTotalSize, Qt::DescendingOrder);

    ui->labelFolderName->setText(paths::shortenPath(folderPath));
    ui->labelFolderName->setToolTip(folderPath);
    ui->chbTop10->setVisible(extList.size() > 15);

    setTotalInfo(extList);
    ui->types_->setItems(extList);
    connections();
}

DialogContentsList::~DialogContentsList()
{
    delete ui;
}

void DialogContentsList::connections()
{
    connect(ui->chbTop10, &QCheckBox::toggled, this, &DialogContentsList::setItemsVisibility);
    connect(ui->labelFolderName, &ClickableLabel::doubleClicked, this,
            [=]{ paths::browsePath(ui->labelFolderName->toolTip()); });
}

void DialogContentsList::setTotalInfo(const FileTypeList &extList)
{
    ui->labelTotal->setText(QString("Total: %1 types, %2 ")
                                .arg(extList.size())
                                .arg(format::filesNumSize(Files::totalListed(extList))));
}

void DialogContentsList::setItemsVisibility(bool isTop10Checked)
{
    QString __s;

    if (isTop10Checked) {
        ui->types_->hideExtra();
        __s = QStringLiteral(u"Top10: ") + format::filesNumSize(ui->types_->numSizeVisible());
    }
    else {
        ui->types_->showAllItems();
        __s = QStringLiteral(u"Top10");
    }

    ui->chbTop10->setText(__s);
}
