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
    explicit DialogExistingDbs(QWidget *parent = nullptr);
    ~DialogExistingDbs();

private:
    Ui::DialogExistingDbs *ui;
};

#endif // DIALOGEXISTINGDBS_H
