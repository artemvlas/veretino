// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "settingdialog.h"
#include "ui_settingdialog.h"

settingDialog::settingDialog(Settings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settingDialog),
    settings_(settings)
{
    ui->setupUi(this);
    setFixedSize(440,300);
    setWindowIcon(QIcon(":/veretino.png"));

    connect(ui->radioButtonIncludeOnly, &QRadioButton::toggled, this, [=](const bool &disable)
         {ui->ignoreDbFiles->setDisabled(disable); ui->ignoreShaFiles->setDisabled(disable);});

    if (settings->algorithm == QCryptographicHash::Sha1)
        ui->rbSha1->setChecked(true);
    else if (settings->algorithm == QCryptographicHash::Sha256)
        ui->rbSha256->setChecked(true);
    else if (settings->algorithm == QCryptographicHash::Sha512)
        ui->rbSha512->setChecked(true);

    ui->inputExtensions->setText(settings->filter.extensionsList.join(" "));
    ui->radioButtonIncludeOnly->setChecked(settings->filter.includeOnly);

    ui->ignoreDbFiles->setChecked(settings->filter.ignoreDbFiles);
    ui->ignoreShaFiles->setChecked(settings->filter.ignoreShaFiles);

    if (settings->dbPrefix != "checksums")
        ui->inputJsonFileNamePrefix->setText(settings->dbPrefix);
}

void settingDialog::updateSettings()
{
    // algorithm
    if (ui->rbSha1->isChecked())
        settings_->algorithm = QCryptographicHash::Sha1;
    else if (ui->rbSha256->isChecked())
        settings_->algorithm = QCryptographicHash::Sha256;
    else if (ui->rbSha512->isChecked())
        settings_->algorithm = QCryptographicHash::Sha512;

    // dbPrefix
    if (!ui->inputJsonFileNamePrefix->text().isEmpty()) {
        QString fileNamePrefix = ui->inputJsonFileNamePrefix->text();

        QString forbSymb(":*/\?|<>");
        for (int i = 0; i < forbSymb.size(); ++i) {
            fileNamePrefix.replace(forbSymb.at(i), '_');
        }

        settings_->dbPrefix = fileNamePrefix;
    }
    else
        settings_->dbPrefix = "checksums";

    // filters
    ui->radioButtonIgnore->setChecked(ui->inputExtensions->text().isEmpty());
    settings_->filter.includeOnly = !ui->radioButtonIgnore->isChecked();
    settings_->filter.extensionsList = extensionsList();

    settings_->filter.ignoreDbFiles = ui->ignoreDbFiles->isChecked();
    settings_->filter.ignoreShaFiles = ui->ignoreShaFiles->isChecked();
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

settingDialog::~settingDialog()
{
    delete ui;
}
