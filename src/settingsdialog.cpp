/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include <QPushButton>
#include "iconprovider.h"

SettingsDialog::SettingsDialog(Settings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog),
    settings_(settings)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/veretino.png"));

    ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setText("Defaults");

    connect(ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &SettingsDialog::restoreDefaults);
    connect(ui->rbExtVer, &QRadioButton::toggled, this, &SettingsDialog::updateLabelDatabaseFilename);
    connect(ui->cbAddFolderName, &QCheckBox::toggled, this, &SettingsDialog::updateLabelDatabaseFilename);
    connect(ui->inputJsonFileNamePrefix, &QLineEdit::textEdited, this, &SettingsDialog::updateLabelDatabaseFilename);
    connect(ui->radioButtonIncludeOnly, &QRadioButton::toggled, this, [=](const bool &disable)
         {ui->ignoreDbFiles->setDisabled(disable); ui->ignoreShaFiles->setDisabled(disable); setExtensionsColor();});

    ui->cbSaveVerificationDateTime->setToolTip("Checked: after successful verification\n"
                                               "(if all files exist and match the saved checksums),\n"
                                               "the current date/time will be saved in the database file");

    loadSettings(*settings);

    // set tabs icons
    const IconProvider icons(palette());
    ui->tabWidget->setTabIcon(TabDatabase, icons.icon(Icons::Database));
    ui->tabWidget->setTabIcon(TabFilter, icons.icon(Icons::Filter));
}

void SettingsDialog::loadSettings(const Settings &settings)
{
    switch (settings.algorithm)
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

    ui->cbLastPath->setChecked(settings.restoreLastPathOnStartup);

    // Tab Database
    if (settings.dbPrefix == defaults.dbPrefix)
        ui->inputJsonFileNamePrefix->clear();
    else
        ui->inputJsonFileNamePrefix->setText(settings.dbPrefix);

    ui->cbAddFolderName->setChecked(settings.addWorkDirToFilename);
    ui->cbSaveVerificationDateTime->setChecked(settings.saveVerificationDateTime);
    settings.isLongExtension ? ui->rbExtVerJson->setChecked(true) : ui->rbExtVer->setChecked(true);

    updateLabelDatabaseFilename();

    // Tab Filter
    setExtensionsColor();
    ui->inputExtensions->setText(settings.filter.extensionsList.join(" "));
    settings.filter.isFilter(FilterRule::Include) ? ui->radioButtonIncludeOnly->setChecked(true) : ui->radioButtonIgnore->setChecked(true);

    ui->ignoreDbFiles->setChecked(settings.filter.ignoreDbFiles);
    ui->ignoreShaFiles->setChecked(settings.filter.ignoreShaFiles);

    // Select open Tab
    Tabs curTab = ui->inputExtensions->text().isEmpty() ? TabDatabase : TabFilter;
    ui->tabWidget->setCurrentIndex(curTab);
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

    settings_->restoreLastPathOnStartup = ui->cbLastPath->isChecked();

    // database filename
    settings_->dbPrefix = ui->inputJsonFileNamePrefix->text().isEmpty() ? defaults.dbPrefix
                                                                        : format::simplifiedChars(ui->inputJsonFileNamePrefix->text());

    settings_->isLongExtension = ui->rbExtVerJson->isChecked();
    settings_->addWorkDirToFilename = ui->cbAddFolderName->isChecked();

    // filters
    ui->radioButtonIgnore->setChecked(ui->inputExtensions->text().isEmpty());

    if (ui->inputExtensions->text().isEmpty())
        settings_->filter.clearFilter();
    else
        ui->radioButtonIgnore->isChecked() ? settings_->filter.setFilter(FilterRule::Ignore, extensionsList())
                                           : settings_->filter.setFilter(FilterRule::Include, extensionsList());

    settings_->filter.ignoreDbFiles = ui->ignoreDbFiles->isChecked();
    settings_->filter.ignoreShaFiles = ui->ignoreShaFiles->isChecked();
    settings_->saveVerificationDateTime = ui->cbSaveVerificationDateTime->isChecked();
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

void SettingsDialog::updateLabelDatabaseFilename()
{
    QString prefix = ui->inputJsonFileNamePrefix->text().isEmpty() ? defaults.dbPrefix : format::simplifiedChars(ui->inputJsonFileNamePrefix->text());
    QString folderName = ui->cbAddFolderName->isChecked() ? "@FolderName" : QString();
    QString extension = defaults.dbFileExtension(ui->rbExtVerJson->isChecked());

    ui->labelDatabaseFilename->setText(format::composeDbFileName(prefix, folderName, extension));
}

void SettingsDialog::restoreDefaults()
{
    loadSettings(defaults);
}

void SettingsDialog::setExtensionsColor()
{
    ui->inputExtensions->setStyleSheet(QString("QLineEdit { %1 }").arg(format::coloredText(ui->radioButtonIgnore->isChecked())));
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
