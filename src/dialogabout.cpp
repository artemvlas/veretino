/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogabout.h"
#include "ui_dialogabout.h"
#include "iconprovider.h"

DialogAbout::DialogAbout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogAbout)
{
    ui->setupUi(this);
    setFixedSize(400, 200);
    setWindowIcon(IconProvider::appIcon());
    ui->labelPix->setPixmap(IconProvider::appIcon().pixmap(100, 100));

    ui->labelInfo->setText(QString("Veretino %1\nBuilt: %2\nQt at run-time: %3\n\nFree and open-source software\nGNU General Public License v3")
                               .arg(APP_VERSION, __DATE__, qVersion()));

    ui->labelAuthor->setTextFormat(Qt::RichText);
    ui->labelAuthor->setOpenExternalLinks(true);
    ui->labelAuthor->setText("<center>Author/Developer:<div><center>Artem Vlasenko: <a href='mailto:artemvlas@proton.me?subject=Veretino'>artemvlas@proton.me</a>"
                             "<div><center><a href='https://github.com/artemvlas/veretino'>GitHub</a>");
}

DialogAbout::~DialogAbout()
{
    delete ui;
}
