// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
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
private:
    Ui::SettingsDialog *ui;
    QStringList extensionsList(); // return a list of extensions from input
    Settings *settings_;
};

#endif // SETTINGSDIALOG_H