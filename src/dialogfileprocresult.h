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

private:
    void setModeMatched();
    void setModeMismatched();
    void setModeComputed();
    void setModeCopied();
    void setModeStored();
    void setModeUnstored();
    void addButtonCopy();
    void addButtonSave();
    void setIcon(const QIcon &icon);
    void makeSumFile();
    void setFileName(const QString &filePath);
    void showLineSizeAlgo();
    void hideLineSizeAlgo();

    Ui::DialogFileProcResult *ui;
    IconProvider icons_;
    QString filePath_;
    FileValues values_;
};

#endif // DIALOGFILEPROCRESULT_H
