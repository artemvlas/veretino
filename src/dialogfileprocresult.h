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
    explicit DialogFileProcResult(const QString &fileName, const FileValues &values, QWidget *parent = nullptr);
    ~DialogFileProcResult();
    enum ClickedButton {Undefined, Copy, Save};
    ClickedButton clickedButton = Undefined;

private:
    void setInfo(const QString &fileName, const FileValues &values);
    void setModeCalculated();

    Ui::DialogFileProcResult *ui;
    IconProvider icons_;
};

#endif // DIALOGFILEPROCRESULT_H
