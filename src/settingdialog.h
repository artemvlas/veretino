// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef SETTINGDIALOG_H
#define SETTINGDIALOG_H

#include <QDialog>
#include "QVariant"

namespace Ui {
class settingDialog;
}

class settingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit settingDialog(const QVariantMap &settingsMap, QWidget *parent = nullptr);
    ~settingDialog();
    QVariantMap getSettings();
private:
    Ui::settingDialog *ui;
    QVariantMap settings;
    QStringList extensionsList(); // return a list of extensions from input
};

#endif // SETTINGDIALOG_H
