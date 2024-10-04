#ifndef DIALOGDBCREATION_H
#define DIALOGDBCREATION_H

#include <QDialog>
#include <QKeyEvent>
#include "iconprovider.h"
#include "treewidgetitem.h"
#include "files.h"
#include "settings.h"

namespace Ui {
class DialogDbCreation;
}

class DialogDbCreation : public QDialog
{
    Q_OBJECT

public:
    explicit DialogDbCreation(const QString &folderPath,
                              const QList<ExtNumSize> &extList,
                              QWidget *parent = nullptr);
    ~DialogDbCreation();

    enum FilterCreation { FC_Disabled, FC_Enabled };

    void setSettings(Settings *settings);
    void updateSettings();
    void setFilterCreation(FilterCreation mode);
    FilterRule resultFilter();

private:
    Ui::DialogDbCreation *ui;

    void setDbConfig();
    void connections();
    void updateLabelDbFilename();

    void setTotalInfo();
    void setItemsVisibility(bool isTop10Checked);
    void setCheckboxesVisible(bool visible);
    void clearChecked();
    void updateViewMode();
    void activateItem(QTreeWidgetItem *t_item);
    void updateFilterDisplay();
    void updateLabelFilterExtensions();
    void updateLabelTotalFiltered();
    bool itemsContain(int state) const;

    QList<ExtNumSize> extList_;
    FilterCreation mode_ = FC_Disabled;

    Settings *settings_ = nullptr;
    IconProvider icons_;
    QString workDir_;


protected:
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;
};

#endif // DIALOGDBCREATION_H
