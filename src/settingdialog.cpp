#include "settingdialog.h"
#include "ui_settingdialog.h"

settingDialog::settingDialog(const Settings &settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::settingDialog),
    settings_(settings)
{
    ui->setupUi(this);
    this->setFixedSize(440,300);
    this->setWindowIcon(QIcon(":/veretino.png"));

    connect(ui->radioButtonIncludeOnly, &QRadioButton::toggled, this, [=](const bool &disable)
         {ui->ignoreDbFiles->setDisabled(disable); ui->ignoreShaFiles->setDisabled(disable);});

    if (settings.algorithm == QCryptographicHash::Sha1)
        ui->rbSha1->setChecked(true);
    else if (settings.algorithm == QCryptographicHash::Sha256)
        ui->rbSha256->setChecked(true);
    else if (settings.algorithm == QCryptographicHash::Sha512)
        ui->rbSha512->setChecked(true);

    ui->inputExtensions->setText(settings.filter.extensionsList.join(" "));
    ui->radioButtonIncludeOnly->setChecked(settings.filter.includeOnly);

    ui->ignoreDbFiles->setChecked(settings_.filter.ignoreDbFiles);
    ui->ignoreShaFiles->setChecked(settings_.filter.ignoreShaFiles);

    ui->inputJsonFileNamePrefix->setText(settings.dbPrefix);
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

Settings settingDialog::getSettings()
{
    // algorithm
    if (ui->rbSha1->isChecked())
        settings_.algorithm = QCryptographicHash::Sha1;
    else if (ui->rbSha256->isChecked())
        settings_.algorithm = QCryptographicHash::Sha256;
    else if (ui->rbSha512->isChecked())
        settings_.algorithm = QCryptographicHash::Sha512;

    // dbPrefix
    if (!ui->inputJsonFileNamePrefix->text().isEmpty()) {
        QString fileNamePrefix = ui->inputJsonFileNamePrefix->text();

        QString forbSymb(":*/\?|<>");
        for (int i = 0; i < forbSymb.size(); ++i) {
            fileNamePrefix.replace(forbSymb.at(i), '_');
        }

        settings_.dbPrefix = fileNamePrefix;
    }
    else
        settings_.dbPrefix = "checksums";

    // filters
    ui->radioButtonIgnore->setChecked(ui->inputExtensions->text().isEmpty());
    settings_.filter.includeOnly = !ui->radioButtonIgnore->isChecked();
    settings_.filter.extensionsList = extensionsList();

    settings_.filter.ignoreDbFiles = ui->ignoreDbFiles->isChecked();
    settings_.filter.ignoreShaFiles = ui->ignoreShaFiles->isChecked();

    return settings_;
}
