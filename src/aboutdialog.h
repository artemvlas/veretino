// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>

namespace Ui {
class aboutDialog;
}

class aboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit aboutDialog(QWidget *parent = nullptr);
    ~aboutDialog();

private:
    Ui::aboutDialog *ui;
};

#endif // ABOUTDIALOG_H
