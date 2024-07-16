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
    //setFixedSize(400, 200);
    setWindowIcon(IconProvider::appIcon());
    ui->labelPix->setPixmap(IconProvider::appIcon().pixmap(100, 100));

    ui->labelAbout->setTextFormat(Qt::RichText);
    ui->labelAbout->setOpenExternalLinks(true);
    ui->labelAbout->setText(QString("<b>%1</b>"
                                    "<br>Built: %2"
                                    "<br>Qt at run-time: %3"
                                    "<br>"
                                    "<br>Free and open-source software"
                                    "<br>GNU General Public License v3"
                                    "<br>"
                                    "<br>Home Page: <a href='https://github.com/artemvlas/veretino'>GitHub</a>")
                                .arg(APP_NAME_VERSION, __DATE__, qVersion()));

    ui->labelAuthor->setTextFormat(Qt::RichText);
    ui->labelAuthor->setOpenExternalLinks(true);
    ui->labelAuthor->setText("<center>Author/Developer: Artem Vlasenko"
                             "<div><center>"
                             "<a href='mailto:artemvlas@proton.me?subject=Veretino'>artemvlas@proton.me</a>"
                             "<div><center>"
                             "<a href='https://github.com/artemvlas'>GitHub</a>"
                             "<br>"
                             "<br>"
                             "<br>"
                             "Thanks to the Breeze Theme authors for the icons.");
}

DialogAbout::~DialogAbout()
{
    delete ui;
}
