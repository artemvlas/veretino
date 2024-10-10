/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DIALOGSETTINGS_H
#define DIALOGSETTINGS_H

#include <QDialog>
#include <QVariant>
#include "settings.h"

namespace Ui {
class DialogSettings;
}

class DialogSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSettings(Settings *settings, QWidget *parent = nullptr);
    ~DialogSettings();
    void updateSettings();
    enum Tabs { TabMain, TabDatabase, TabExtra };

private:
    void loadSettings(const Settings &settings);
    void restoreDefaults();
    void updateLabelDatabaseFilename();

    Ui::DialogSettings *ui;
    Settings *settings_;
    const Settings defaults;

}; // class DialogSettings

#endif // DIALOGSETTINGS_H
