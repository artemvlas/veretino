/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogexistingdbs.h"
#include "ui_dialogexistingdbs.h"
#include <QPushButton>
#include "iconprovider.h"

DialogExistingDbs::DialogExistingDbs(const QStringList &fileList, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogExistingDbs)
{
    ui->setupUi(this);
    setWindowIcon(IconProvider::appIcon());

    ui->listWidget->addItems(fileList);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral(u"Open"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral(u"Create New"));

    connect(ui->listWidget, &QListWidget::doubleClicked, this, &DialogExistingDbs::accept);
}

DialogExistingDbs::~DialogExistingDbs()
{
    delete ui;
}

QString DialogExistingDbs::curFile()
{
    return ui->listWidget->currentItem()->text();
}
