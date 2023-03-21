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

    int shaType = settings.value("shaType").toInt();
    if (shaType == 1)
        ui->rbSha1->setChecked(true);
    else if (shaType == 256)
        ui->rbSha256->setChecked(true);
    else if (shaType == 512)
        ui->rbSha512->setChecked(true);

    if (settings.value("ignoredExtensions").isValid() && !settings.value("ignoredExtensions").toStringList().isEmpty()) {
        ui->inputExtensions->setText(settings.value("ignoredExtensions").toStringList().join(" "));
        ui->radioButtonIgnore->setChecked(true);
    }
    else if (settings.value("onlyExtensions").isValid() && !settings.value("onlyExtensions").toStringList().isEmpty()) {
        ui->inputExtensions->setText(settings.value("onlyExtensions").toStringList().join(" "));
        ui->radioButtonOnly->setChecked(true);
    }

    if (settings.value("ignoreDbFiles").isValid()) {
        ui->ignoreDbFiles->setChecked(settings.value("ignoreDbFiles").toBool());
    }
    if (settings.value("ignoreShaFiles").isValid()){
        ui->ignoreShaFiles->setChecked(settings.value("ignoreShaFiles").toBool());
    }

    if (settings.value("dbPrefix").isValid())
        ui->inputJsonFileNamePrefix->setText(settings.value("dbPrefix").toString());

}

settingDialog::~settingDialog()
{
    delete ui;
}

QStringList settingDialog::extensionsList()
{
    if (!ui->inputExtensions->text().isEmpty()) {
        QString ignoreExtensions = ui->inputExtensions->text().toLower();
        ignoreExtensions.remove('*');
        ignoreExtensions.replace(" ."," ");
        ignoreExtensions.replace(' ',',');

        if (ignoreExtensions.startsWith('.'))
            ignoreExtensions.remove(0, 1);

        QStringList ext = ignoreExtensions.split(',');
        ext.removeDuplicates();
        ext.removeOne("");
        return ext;
    }
    else
        return QStringList();
}

QVariantMap settingDialog::getSettings()
{
    if (ui->rbSha1->isChecked())
        settings["shaType"] = 1;
    else if (ui->rbSha256->isChecked())
        settings["shaType"] = 256;
    else if (ui->rbSha512->isChecked())
        settings["shaType"] = 512;

    if (!ui->inputJsonFileNamePrefix->text().isEmpty()) {
        settings["dbPrefix"] = ui->inputJsonFileNamePrefix->text();
    }
    else
        settings.remove("dbPrefix");

    if (!ui->inputExtensions->text().isEmpty()) {
        if (ui->radioButtonIgnore->isChecked()) {
            settings["ignoredExtensions"] = extensionsList();
            settings.remove("onlyExtensions");
        }
        else if (ui->radioButtonOnly->isChecked()) {
            settings["onlyExtensions"] = extensionsList();
            settings.remove("ignoredExtensions");
        }
    }
    else {
        settings.remove("ignoredExtensions");
        settings.remove("onlyExtensions");
    }

    settings["ignoreDbFiles"] = ui->ignoreDbFiles->isChecked();
    settings["ignoreShaFiles"] = ui->ignoreShaFiles->isChecked();

    //qDebug()<< "settingDialog::getSettings() | " << settings;

    return settings;
}
