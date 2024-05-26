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
    setWindowIcon(IconProvider::iconVeretino);

    ui->buttonBox->button(QDialogButtonBox::RestoreDefaults)->setText("Defaults");
    ui->comboBoxPresets->addItems(filterPresetsList);

    connect(ui->buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked, this, &DialogSettings::restoreDefaults);
    connect(ui->rbExtVer, &QRadioButton::toggled, this, &DialogSettings::updateLabelDatabaseFilename);
    connect(ui->cbAddFolderName, &QCheckBox::toggled, this, &DialogSettings::updateLabelDatabaseFilename);
    connect(ui->inputJsonFileNamePrefix, &QLineEdit::textEdited, this, &DialogSettings::updateLabelDatabaseFilename);
    connect(ui->comboBoxPresets, qOverload<int>(&QComboBox::activated), this, &DialogSettings::setFilterPreset);
    connect(ui->inputExtensions, &QLineEdit::textEdited, this, qOverload<>(&DialogSettings::setComboBoxFpIndex));
    connect(ui->rbInclude, &QRadioButton::toggled, this, &DialogSettings::handleFilterMode);

    ui->cbSaveVerificationDateTime->setToolTip("Checked: after successful verification\n"
                                               "(if all files exist and match the saved checksums),\n"
                                               "the current date/time will be saved in the database file");

    loadSettings(*settings);

    // set tabs icons
    const IconProvider icons(palette());
    ui->tabWidget->setTabIcon(TabMain, icons.icon(Icons::Configure));
    ui->tabWidget->setTabIcon(TabDatabase, icons.icon(Icons::Database));
    ui->tabWidget->setTabIcon(TabFilter, icons.icon(Icons::Filter));
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

    // Tab Database
    if (settings.dbPrefix == defaults.dbPrefix)
        ui->inputJsonFileNamePrefix->clear();
    else
        ui->inputJsonFileNamePrefix->setText(settings.dbPrefix);

    ui->cbAddFolderName->setChecked(settings.addWorkDirToFilename);
    ui->cbSaveVerificationDateTime->setChecked(settings.saveVerificationDateTime);
    ui->cbColoredItems->setChecked(settings.coloredDbItems);
    settings.isLongExtension ? ui->rbExtVerJson->setChecked(true) : ui->rbExtVer->setChecked(true);

    updateLabelDatabaseFilename();

    // Tab Filter
    setFilterRule(settings.filter);

    // Select open Tab
    Tabs curTab = ui->inputExtensions->text().isEmpty() ? TabMain : TabFilter;
    ui->tabWidget->setCurrentIndex(curTab);
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

    // database
    settings_->dbPrefix = ui->inputJsonFileNamePrefix->text().isEmpty() ? defaults.dbPrefix
                                                                        : format::simplifiedChars(ui->inputJsonFileNamePrefix->text());

    settings_->isLongExtension = ui->rbExtVerJson->isChecked();
    settings_->addWorkDirToFilename = ui->cbAddFolderName->isChecked();
    settings_->saveVerificationDateTime = ui->cbSaveVerificationDateTime->isChecked();
    settings_->coloredDbItems = ui->cbColoredItems->isChecked();

    // filters
    settings_->filter = getCurrentFilter();
    cleanUpExtList();
}

QStringList DialogSettings::extensionsList() const
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

void DialogSettings::updateLabelDatabaseFilename()
{
    QString prefix = ui->inputJsonFileNamePrefix->text().isEmpty() ? defaults.dbPrefix : format::simplifiedChars(ui->inputJsonFileNamePrefix->text());
    QString folderName = ui->cbAddFolderName->isChecked() ? "@FolderName" : QString();
    QString extension = defaults.dbFileExtension(ui->rbExtVerJson->isChecked());

    ui->labelDatabaseFilename->setText(format::composeDbFileName(prefix, folderName, extension));
}

void DialogSettings::restoreDefaults()
{
    loadSettings(defaults);
}

void DialogSettings::setExtensionsColor()
{
    ui->inputExtensions->setStyleSheet(format::coloredText("QLineEdit", ui->radioButtonIgnore->isChecked()));
}

void DialogSettings::setComboBoxFpIndex()
{
    setComboBoxFpIndex(getCurrentFilter());
}

void DialogSettings::setComboBoxFpIndex(const FilterRule &filter)
{
    FilterPreset preset = PresetCustom;

    if (filter.isFilter(FilterRule::Include)) {
        if (filter.extensionsList == listPresetDocuments)
            preset = PresetDocuments;
        else if (filter.extensionsList == listPresetPictures)
            preset = PresetPictures;
        else if (filter.extensionsList == listPresetMusic)
            preset = PresetMusic;
        else if (filter.extensionsList == listPresetVideos)
            preset = PresetVideos;
    }
    else if (filter.isFilter(FilterRule::Ignore)) {
        if (filter.extensionsList == listPresetIgnoreTriflings)
            preset = PresetIgnoreTriflings;
    }

    ui->comboBoxPresets->setCurrentIndex(preset);
}

void DialogSettings::setFilterRule(const FilterRule &filter)
{
    ui->inputExtensions->setText(filter.extensionsList.join(" "));
    filter.isFilter(FilterRule::Include) ? ui->rbInclude->setChecked(true) : ui->radioButtonIgnore->setChecked(true);

    ui->ignoreDbFiles->setChecked(filter.ignoreDbFiles);
    ui->ignoreShaFiles->setChecked(filter.ignoreShaFiles);

    setExtensionsColor();
    setComboBoxFpIndex(filter);
}

void DialogSettings::setFilterPreset(int presetIndex)
{
    if (presetIndex > PresetCustom)
        setFilterRule(selectPresetFilter(static_cast<FilterPreset>(presetIndex)));
}

FilterRule DialogSettings::selectPresetFilter(const FilterPreset preset)
{
    switch (preset) {
    case PresetDocuments:
        return FilterRule(FilterRule::Include, listPresetDocuments);
    case PresetPictures:
        return FilterRule(FilterRule::Include, listPresetPictures);
    case PresetMusic:
        return FilterRule(FilterRule::Include, listPresetMusic);
    case PresetVideos:
        return FilterRule(FilterRule::Include, listPresetVideos);
    case PresetIgnoreTriflings:
        return FilterRule(FilterRule::Ignore, listPresetIgnoreTriflings);
    default:
        return FilterRule();
    }
}

FilterRule DialogSettings::getCurrentFilter() const
{
    FilterRule curFilter;
    curFilter.ignoreDbFiles = ui->ignoreDbFiles->isChecked();
    curFilter.ignoreShaFiles = ui->ignoreShaFiles->isChecked();

    if (!ui->inputExtensions->text().isEmpty()) {
        ui->radioButtonIgnore->isChecked() ? curFilter.setFilter(FilterRule::Ignore, extensionsList())
                                           : curFilter.setFilter(FilterRule::Include, extensionsList());
    }

    return curFilter;
}

void DialogSettings::handleFilterMode()
{
    const bool isInculde = ui->rbInclude->isChecked();

    ui->ignoreDbFiles->setDisabled(isInculde);
    ui->ignoreShaFiles->setDisabled(isInculde);

    setExtensionsColor();
    setComboBoxFpIndex();
}

void DialogSettings::cleanUpExtList()
{
    if (!settings_->filter.extensionsList.isEmpty()) {
        QStringList list;
        if (!settings_->filter.isFilter(FilterRule::Include)) {
            if (settings_->filter.ignoreShaFiles)
                list.append({ "sha1", "sha256", "sha512" });
            if (settings_->filter.ignoreDbFiles)
                list.append({ "ver", "ver.json" });
        }

        if (!list.isEmpty()) {
            foreach (const QString &str, list) {
                settings_->filter.extensionsList.removeOne(str);
            }
        }
    }
}

DialogSettings::~DialogSettings()
{
    delete ui;
}
