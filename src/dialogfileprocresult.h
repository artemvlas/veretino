/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef DIALOGFILEPROCRESULT_H
#define DIALOGFILEPROCRESULT_H

#include <QDialog>
#include "files.h"
#include "iconprovider.h"

namespace Ui {
class DialogFileProcResult;
}

class DialogFileProcResult : public QDialog
{
    Q_OBJECT

public:
    explicit DialogFileProcResult(const QString &filePath, const FileValues &values, QWidget *parent = nullptr);
    ~DialogFileProcResult();
    enum ClickedButton {Undefined, Copy, Save};
    ClickedButton clickedButton = Undefined;

private:
    void setInfo(const QString &filePath, const FileValues &values);
    void setModeCalculated();
    void setModeUnstored();

    Ui::DialogFileProcResult *ui;
    IconProvider icons_;
};

#endif // DIALOGFILEPROCRESULT_H
