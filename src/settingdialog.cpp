#include "settingdialog.h"
#include "ui_settingdialog.h"
#include "QDebug"

settingDialog::settingDialog(const QVariantMap &settingsMap, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settingDialog)
{
    ui->setupUi(this);
    this->setFixedSize(410,220);
    this->setWindowIcon(QIcon(":/veretino.png"));

    settings = settingsMap;

    int shaType = settings["shaType"].toInt();
    if (shaType == 1)
        ui->rbSha1->setChecked(true);
    else if (shaType == 256)
        ui->rbSha256->setChecked(true);
    else if (shaType == 512)
        ui->rbSha512->setChecked(true);

    if(settings["extensions"].isValid())
        ui->inputFilterExtensions->setText(settings["extensions"].toStringList().join(" "));

    if(settings["filterDbFiles"].isValid()) {
        ui->filterDbFiles->setChecked(settings["filterDbFiles"].toBool());
    }
    if(settings["filterShaFiles"].isValid()){
        ui->filterShaFiles->setChecked(settings["filterShaFiles"].toBool());
    }

}

settingDialog::~settingDialog()
{
    delete ui;
}

QVariantMap settingDialog::getSettings()
{
    if(ui->rbSha1->isChecked())
        settings["shaType"] = 1;
    else if (ui->rbSha256->isChecked())
        settings["shaType"] = 256;
    else if (ui->rbSha512->isChecked())
        settings["shaType"] = 512;

    if (ui->inputFilterExtensions->text() != nullptr) {
        QString filterExtensions = ui->inputFilterExtensions->text().toLower();
        filterExtensions.replace("*","");
        filterExtensions.replace(".","");

        QStringList ext = filterExtensions.split(" ");
        settings["extensions"] = ext;
    }
    else
        settings["extensions"] = QStringList();


    settings["filterDbFiles"] = ui->filterDbFiles->isChecked();
    settings["filterShaFiles"] = ui->filterShaFiles->isChecked();

    return settings;
}
