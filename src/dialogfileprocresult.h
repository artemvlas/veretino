/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DIALOGFILEPROCRESULT_H
#define DIALOGFILEPROCRESULT_H

#include <QDialog>
#include "filevalues.h"
#include "iconprovider.h"

namespace Ui {
class DialogFileProcResult;
}

class DialogFileProcResult : public QDialog
{
    Q_OBJECT

public:
    explicit DialogFileProcResult(const QString &filePath,
                                  const FileValues &values, QWidget *parent = nullptr);
    ~DialogFileProcResult();

private:
    void setFileName(const QString &filePath);
    void setIcon(Icons icon);
    void setIcon(FileStatus status);
    void setExtLineVisible(bool visible); // Size: ...  Algorithm: ...
    void hideLineSizeAlgo();
    void addButtonCopy();
    void addButtonSave();
    void addButtonOverwrite();
    void makeSumFile();
    void makeSumFileUnforce(); // checks the existence of the digest file first
    void verify();

    void setModeMatched();
    void setModeMismatched();
    void setModeComputed();
    void setModeCopied();
    void setModeStored();
    void setModeUnstored();
    void setModeDigestFileExists();
    void setModeReadError();

    QString digestFilePath() const;

    Ui::DialogFileProcResult *m_ui;
    IconProvider m_icons;
    QString m_filePath;
    FileValues m_values;
}; // class DialogFileProcResult

#endif // DIALOGFILEPROCRESULT_H
