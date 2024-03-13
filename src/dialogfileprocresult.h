#ifndef DIALOGFILEPROCRESULT_H
#define DIALOGFILEPROCRESULT_H

#include <QDialog>
#include "files.h"

namespace Ui {
class DialogFileProcResult;
}

class DialogFileProcResult : public QDialog
{
    Q_OBJECT

public:
    explicit DialogFileProcResult(const QString &fileName, const FileValues &values, QWidget *parent = nullptr);
    ~DialogFileProcResult();

private:
    Ui::DialogFileProcResult *ui;
    void setInfo(const QString &fileName, const FileValues &values);
};

#endif // DIALOGFILEPROCRESULT_H
