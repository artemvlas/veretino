/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(Settings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog),
    settings_(settings)
{
    ui->setupUi(this);
    setFixedSize(440,300);
    setWindowIcon(QIcon(":/veretino.png"));

    connect(ui->radioButtonIncludeOnly, &QRadioButton::toggled, this, [=](const bool &disable)
         {ui->ignoreDbFiles->setDisabled(disable); ui->ignoreShaFiles->setDisabled(disable);});

    switch (settings->algorithm)
    {
    case QCryptographicHash::Sha1:
        ui->rbSha1->setChecked(true);
        break;
    case QCryptographicHash::Sha256:
        ui->rbSha256->setChecked(true);
        break;
    case QCryptographicHash::Sha512:
        ui->rbSha512->setChecked(true);
        break;
    default:
        break;
    }

    ui->inputExtensions->setText(settings->filter.extensionsList.join(" "));
    ui->radioButtonIncludeOnly->setChecked(settings->filter.isFilter(FilterRule::Include));

    ui->ignoreDbFiles->setChecked(settings->filter.ignoreDbFiles);
    ui->ignoreShaFiles->setChecked(settings->filter.ignoreShaFiles);

    if (settings->dbPrefix != "checksums")
        ui->inputJsonFileNamePrefix->setText(settings->dbPrefix);
}

void SettingsDialog::updateSettings()
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

    if (ui->inputExtensions->text().isEmpty())
        settings_->filter.clearFilter();
    else
        ui->radioButtonIgnore->isChecked() ? settings_->filter.setFilter(FilterRule::Ignore, extensionsList())
                                           : settings_->filter.setFilter(FilterRule::Include, extensionsList());


    settings_->filter.ignoreDbFiles = ui->ignoreDbFiles->isChecked();
    settings_->filter.ignoreShaFiles = ui->ignoreShaFiles->isChecked();
}

QStringList SettingsDialog::extensionsList()
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

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
