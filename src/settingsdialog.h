/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QVariant>
#include "tools.h"

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(Settings *settings, QWidget *parent = nullptr);
    ~SettingsDialog();
    void updateSettings();
    enum Tabs {TabDatabase, TabFilter};

private:
    void loadSettings(const Settings &settings);
    void restoreDefaults();
    void updateLabelDatabaseFilename();

    Ui::SettingsDialog *ui;
    QStringList extensionsList(); // return a list of extensions from input
    Settings *settings_;
};

#endif // SETTINGSDIALOG_H
