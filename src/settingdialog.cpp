#include "settingdialog.h"
#include "ui_settingdialog.h"
#include "QDebug"

settingDialog::settingDialog(const QVariantMap &settingsMap, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settingDialog)
{
    ui->setupUi(this);
    this->setFixedSize(440,220);
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
        ui->inputIgnoreExtensions->setText(settings["extensions"].toStringList().join(" "));

    if(settings["ignoreDbFiles"].isValid()) {
        ui->ignoreDbFiles->setChecked(settings["ignoreDbFiles"].toBool());
    }
    if(settings["ignoreShaFiles"].isValid()){
        ui->ignoreShaFiles->setChecked(settings["ignoreShaFiles"].toBool());
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

    if (ui->inputIgnoreExtensions->text() != nullptr) {
        QString ignoreExtensions = ui->inputIgnoreExtensions->text().toLower();
        ignoreExtensions.replace("*","");
        ignoreExtensions.replace(".","");

        QStringList ext = ignoreExtensions.split(" ");
        settings["extensions"] = ext;
    }
    else
        settings["extensions"] = QStringList();


    settings["ignoreDbFiles"] = ui->ignoreDbFiles->isChecked();
    settings["ignoreShaFiles"] = ui->ignoreShaFiles->isChecked();

    return settings;
}
