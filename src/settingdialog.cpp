#include "settingdialog.h"
#include "ui_settingdialog.h"
#include "QDebug"

settingDialog::settingDialog(const QVariantMap &settingsMap, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settingDialog)
{
    ui->setupUi(this);
    this->setFixedSize(440,300);
    this->setWindowIcon(QIcon(":/veretino.png"));

    connect(ui->radioButtonOnly, &QRadioButton::toggled, this, [=](const bool &disable){ui->ignoreDbFiles->setDisabled(disable); ui->ignoreShaFiles->setDisabled(disable);});

    settings = settingsMap;

    int shaType = settings["shaType"].toInt();
    if (shaType == 1)
        ui->rbSha1->setChecked(true);
    else if (shaType == 256)
        ui->rbSha256->setChecked(true);
    else if (shaType == 512)
        ui->rbSha512->setChecked(true);

    if (settings["ignoredExtensions"].isValid() && !settings["ignoredExtensions"].toStringList().isEmpty()) {
        ui->inputExtensions->setText(settings["ignoredExtensions"].toStringList().join(" "));
        ui->radioButtonIgnore->setChecked(true);
    }
    else if (settings["onlyExtensions"].isValid() && !settings["onlyExtensions"].toStringList().isEmpty()) {
        ui->inputExtensions->setText(settings["onlyExtensions"].toStringList().join(" "));
        ui->radioButtonOnly->setChecked(true);
    }

    if(settings["ignoreDbFiles"].isValid()) {
        ui->ignoreDbFiles->setChecked(settings["ignoreDbFiles"].toBool());
    }
    if(settings["ignoreShaFiles"].isValid()){
        ui->ignoreShaFiles->setChecked(settings["ignoreShaFiles"].toBool());
    }

    if(settings["dbPrefix"].isValid())
        ui->inputJsonFileNamePrefix->setText(settings["dbPrefix"].toString());

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

    if (ui->inputJsonFileNamePrefix->text() != nullptr) {
        settings["dbPrefix"] = ui->inputJsonFileNamePrefix->text();
    }

    if (ui->inputExtensions->text() != nullptr) {
        QString ignoreExtensions = ui->inputExtensions->text().toLower();
        ignoreExtensions.remove('*');
        ignoreExtensions.replace(" ."," ");
        ignoreExtensions.replace(' ',',');

        if (ignoreExtensions.startsWith('.'))
            ignoreExtensions.remove(0, 1);

        QStringList ext = ignoreExtensions.split(',');
        ext.removeDuplicates();
        ext.removeOne("");

        if (ui->radioButtonIgnore->isChecked()) {
            settings["ignoredExtensions"] = ext;
            settings["onlyExtensions"] = QStringList();;
        }
        else if (ui->radioButtonOnly->isChecked()) {
            settings["onlyExtensions"] = ext;
            settings["ignoredExtensions"] = QStringList();;
        }
    }
    else {
        settings["ignoredExtensions"] = QStringList();
        settings["onlyExtensions"] = QStringList();
    }

    settings["ignoreDbFiles"] = ui->ignoreDbFiles->isChecked();
    settings["ignoreShaFiles"] = ui->ignoreShaFiles->isChecked();

    //qDebug()<< "settingDialog::getSettings() | " << settings;

    return settings;
}
