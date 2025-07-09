/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "dialogsettings.h"
#include "ui_dialogsettings.h"
#include <QPushButton>
#include "iconprovider.h"
#include "tools.h"
#include <QDebug>

DialogSettings::DialogSettings(Settings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSettings),
    settings_(settings)
{
    ui->setupUi(this);
    setWindowIcon(IconProvider::appIcon());

    ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setText(QStringLiteral(u"Defaults"));

    connect(ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &DialogSettings::restoreDefaults);
    connect(ui->rbExtVer, &QRadioButton::toggled, this, &DialogSettings::updateLabelDatabaseFilename);
    connect(ui->cbAddFolderName, &QCheckBox::toggled, this, &DialogSettings::updateLabelDatabaseFilename);
    connect(ui->inputJsonFileNamePrefix, &QLineEdit::textEdited, this, &DialogSettings::updateLabelDatabaseFilename);

    ui->cbSaveVerificationDateTime->setToolTip(QStringLiteral(u"Checked: after successful verification\n"
                                                 "(if all files exist and match the stored checksums),\n"
                                                 "the current date/time will be written to the database."));

    ui->cbConsiderDateModified->setToolTip(QStringLiteral(u"During parsing, check the files modified date.\n"
                                                           "Items changed after creating the DB will be marked."));

    loadSettings(*settings);

    // set tabs icons
    const IconProvider icons(palette());
    ui->tabWidget->setTabIcon(TabMain, icons.icon(Icons::Configure));
    ui->tabWidget->setTabIcon(TabDatabase, icons.icon(Icons::Database));
}

void DialogSettings::loadSettings(const Settings &settings)
{
    switch (settings.algorithm())
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
    ui->cbInstantSaving->setChecked(settings.instantSaving);
    ui->cbConsiderDateModified->setChecked(settings.considerDateModified);
    ui->cbDetectMoved->setChecked(settings.detectMoved);
    ui->cbAllowPaste->setChecked(settings.allowPasteIntoDb);

    // Tab Database
    if (settings.dbPrefix.isEmpty() || (settings.dbPrefix == Lit::s_db_prefix)) {
        ui->inputJsonFileNamePrefix->clear();
    } else {
        ui->inputJsonFileNamePrefix->setText(settings.dbPrefix);
    }

    ui->cbAddFolderName->setChecked(settings.addWorkDirToFilename);
    ui->cbSaveVerificationDateTime->setChecked(settings.saveVerificationDateTime);
    ui->cbDbFlagConst->setChecked(settings.dbFlagConst);
    settings.isLongExtension ? ui->rbExtVerJson->setChecked(true) : ui->rbExtVer->setChecked(true);

    updateLabelDatabaseFilename();

    // Tab Extra
    ui->cbIgnoreDbFiles->setChecked(settings.filter_ignore_db);
    ui->cbIgnoreShaFiles->setChecked(settings.filter_ignore_sha);
    ui->cbIgnoreUnpermitted->setChecked(settings.filter_ignore_unpermitted);
    ui->cbIgnoreSymlinks->setChecked(settings.filter_ignore_symlinks);
}

void DialogSettings::updateSettings()
{
    // algorithm
    if (ui->rbSha1->isChecked())
        settings_->setAlgorithm(QCryptographicHash::Sha1);
    else if (ui->rbSha256->isChecked())
        settings_->setAlgorithm(QCryptographicHash::Sha256);
    else if (ui->rbSha512->isChecked())
        settings_->setAlgorithm(QCryptographicHash::Sha512);

    settings_->restoreLastPathOnStartup = ui->cbLastPath->isChecked();
    settings_->instantSaving = ui->cbInstantSaving->isChecked();
    settings_->considerDateModified = ui->cbConsiderDateModified->isChecked();
    settings_->detectMoved = ui->cbDetectMoved->isChecked();
    settings_->allowPasteIntoDb = ui->cbAllowPaste->isChecked();

    // database
    const QString inpPrefix = ui->inputJsonFileNamePrefix->text();
    settings_->dbPrefix = format::simplifiedChars(inpPrefix);

    settings_->isLongExtension = ui->rbExtVerJson->isChecked();
    settings_->addWorkDirToFilename = ui->cbAddFolderName->isChecked();
    settings_->saveVerificationDateTime = ui->cbSaveVerificationDateTime->isChecked();
    settings_->dbFlagConst = ui->cbDbFlagConst->isChecked();

    // extra filters
    settings_->filter_ignore_db = ui->cbIgnoreDbFiles->isChecked();
    settings_->filter_ignore_sha = ui->cbIgnoreShaFiles->isChecked();
    settings_->filter_ignore_unpermitted = ui->cbIgnoreUnpermitted->isChecked();
    settings_->filter_ignore_symlinks = ui->cbIgnoreSymlinks->isChecked();
}

void DialogSettings::updateLabelDatabaseFilename()
{
    const QString inpPrefix = ui->inputJsonFileNamePrefix->text();

    QString prefix = inpPrefix.isEmpty() ? Lit::s_db_prefix : format::simplifiedChars(inpPrefix);
    QString folderName = ui->cbAddFolderName->isChecked() ? QStringLiteral(u"@FolderName") : QString();
    QString extension = defaults.dbFileExtension(ui->rbExtVerJson->isChecked());

    ui->labelDatabaseFilename->setText(format::composeDbFileName(prefix, folderName, extension));
}

void DialogSettings::restoreDefaults()
{
    loadSettings(defaults);
}

DialogSettings::~DialogSettings()
{
    delete ui;
}
