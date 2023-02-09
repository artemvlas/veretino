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
};

#endif // SETTINGDIALOG_H
