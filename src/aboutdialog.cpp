#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <QIcon>

aboutDialog::aboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::aboutDialog)
{
    ui->setupUi(this);
    this->setFixedSize(400,200);
    this->setWindowIcon(QIcon(":/veretino.png"));
    ui->labelPix->setPixmap(QPixmap(":/veretino.png").scaled(100,100, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    ui->labelInfo->setText("Veretino dev_0.1.5\n\nQt: 5.15\nFree and open-source software\nGNU General Public License v3");

    ui->labelAuthor->setTextFormat(Qt::RichText);
    ui->labelAuthor->setOpenExternalLinks(true);
    ui->labelAuthor->setText("<center>Author/Developer:<div><center>Artem Vlasenko: <a href='mailto:artemvlas@proton.me?subject=Veretino'>artemvlas@proton.me</a>"
                             "<div><center><a href='https://github.com/artemvlas/veretino'>GitHub</a>");

}

aboutDialog::~aboutDialog()
{
    delete ui;
}
