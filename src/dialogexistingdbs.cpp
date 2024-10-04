#include "dialogexistingdbs.h"
#include "ui_dialogexistingdbs.h"

DialogExistingDbs::DialogExistingDbs(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogExistingDbs)
{
    ui->setupUi(this);
}

DialogExistingDbs::~DialogExistingDbs()
{
    delete ui;
}
