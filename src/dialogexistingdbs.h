/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DIALOGEXISTINGDBS_H
#define DIALOGEXISTINGDBS_H

#include <QDialog>

namespace Ui {
class DialogExistingDbs;
}

class DialogExistingDbs : public QDialog
{
    Q_OBJECT

public:
    explicit DialogExistingDbs(const QStringList &fileList, QWidget *parent = nullptr);
    ~DialogExistingDbs();

    QString curFile();

private:
    Ui::DialogExistingDbs *ui;
}; // class DialogExistingDbs

#endif // DIALOGEXISTINGDBS_H
