// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>
#include <QVariant>
#include "tools.h"

namespace Ui {
class settingDialog;
}

class settingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit settingDialog(Settings *settings, QWidget *parent = nullptr);
    ~settingDialog();
    void updateSettings();
private:
    Ui::settingDialog *ui;
    QStringList extensionsList(); // return a list of extensions from input
    Settings *settings_;
};

#endif // SETTINGDIALOG_H
