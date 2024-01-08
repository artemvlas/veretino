/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <QIcon>

aboutDialog::aboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::aboutDialog)
{
    ui->setupUi(this);
    setFixedSize(400,200);
    setWindowIcon(QIcon(":/veretino.png"));
    ui->labelPix->setPixmap(QPixmap(":/veretino.png").scaled(100,100, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    ui->labelInfo->setText(QString("Veretino %1\nBuilt: %2\nQt at run-time: %3\n\nFree and open-source software\nGNU General Public License v3")
                               .arg(APP_VERSION, __DATE__, qVersion()));

    ui->labelAuthor->setTextFormat(Qt::RichText);
    ui->labelAuthor->setOpenExternalLinks(true);
    ui->labelAuthor->setText("<center>Author/Developer:<div><center>Artem Vlasenko: <a href='mailto:artemvlas@proton.me?subject=Veretino'>artemvlas@proton.me</a>"
                             "<div><center><a href='https://github.com/artemvlas/veretino'>GitHub</a>");

}

aboutDialog::~aboutDialog()
{
    delete ui;
}
