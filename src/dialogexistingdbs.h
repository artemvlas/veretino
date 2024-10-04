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
