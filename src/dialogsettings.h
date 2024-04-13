/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef DIALOGSETTINGS_H
#define DIALOGSETTINGS_H

#include <QDialog>
#include <QVariant>
#include "tools.h"

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
    enum Tabs {TabDatabase, TabFilter};

private:
    void loadSettings(const Settings &settings);
    void restoreDefaults();
    void updateLabelDatabaseFilename();
    void setExtensionsColor();

    Ui::DialogSettings *ui;
    QStringList extensionsList(); // return a list of extensions from input
    Settings *settings_;
    const Settings defaults;
};

#endif // DIALOGSETTINGS_H
