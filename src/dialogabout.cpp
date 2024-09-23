/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogabout.h"
#include "ui_dialogabout.h"
#include "iconprovider.h"
#include "tools.h"

DialogAbout::DialogAbout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogAbout)
{
    ui->setupUi(this);
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
                                    "<br>Home Page: <a href='%4'>GitHub</a>")
                                .arg(Lit::s_appNameVersion, __DATE__, qVersion(), Lit::s_webpage));

    ui->labelAuthor->setAlignment(Qt::AlignCenter);
    ui->labelAuthor->setTextFormat(Qt::RichText);
    ui->labelAuthor->setOpenExternalLinks(true);

    ui->labelAuthor->setText("Author/Developer: Artem Vlasenko"
                             "<br>"
                             "<a href='mailto:artemvlas@proton.me?subject=Veretino'>artemvlas@proton.me</a>"
                             "<br>"
                             "<a href='https://github.com/artemvlas'>GitHub</a>"
                             "<br>"
                             "<br>"
                             "<br>"
                             "Thanks to the Breeze Theme creators for the icons.");
}

DialogAbout::~DialogAbout()
{
    delete ui;
}
