// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef DBSTATUSDIALOG_H
#define DBSTATUSDIALOG_H

#include <QDialog>
#include "datacontainer.h"
#include "clickablelabel.h"

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
    Ui::DbStatusDialog *ui;
    const DataContainer *data_;

    enum Tabs{TabContent, TabFilters, TabVerification};
    QStringList infoContent(const DataContainer *data);
    QStringList infoVerification(const DataContainer *data);
    void browsePath(const QString &path);
    void browseWorkDir();
};

#endif // DBSTATUSDIALOG_H
