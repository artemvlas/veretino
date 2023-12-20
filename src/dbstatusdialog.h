#ifndef DBSTATUSDIALOG_H
#define DBSTATUSDIALOG_H

#include <QDialog>
#include "datacontainer.h"

namespace Ui {
class DbStatusDialog;
}

class DbStatusDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DbStatusDialog(const DataContainer *data, QWidget *parent = nullptr);
    ~DbStatusDialog();

private:
    enum Tabs{TabContent, TabFilters, TabVerification};
    QStringList infoContent(const DataContainer *data);
    QStringList infoVerification(const DataContainer *data);
private:
    Ui::DbStatusDialog *ui;
};

#endif // DBSTATUSDIALOG_H
