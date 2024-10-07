#ifndef DIALOGDBCREATION_H
#define DIALOGDBCREATION_H

#include <QDialog>
#include <QKeyEvent>
#include "iconprovider.h"
#include "itemfiletype.h"
#include "files.h"
#include "settings.h"
#include "widgetfiletypes.h"

namespace Ui {
class DialogDbCreation;
}

class DialogDbCreation : public QDialog
{
    Q_OBJECT

public:
    explicit DialogDbCreation(const QString &folderPath,
                              const FileTypeList &extList,
                              QWidget *parent = nullptr);
    ~DialogDbCreation();

    enum FilterCreation { FC_Disabled, FC_Enabled };

    void setSettings(Settings *settings);
    void updateSettings();
    void setFilterCreation(FilterCreation mode);
    FilterRule resultFilter();

private:
    Ui::DialogDbCreation *ui;

    void connections();
    void setDbConfig();
    void setFilterConfig();
    void restoreLastExts();
    void parseInputedExts();
    void resetView();
    void updateLabelDbFilename();
    void setTotalInfo(const FileTypeList &exts);
    void setItemsVisibility(bool isTop10Checked);
    void setCheckboxesVisible(bool visible);
    void clearChecked();
    void updateViewMode();
    void activateItem(QTreeWidgetItem *t_item);
    void updateFilterDisplay();
    void updateLabelFilterExtensions();
    void updateLabelTotalFiltered();
    bool itemsContain(int state) const;
    void createMenuWidgetTypes(const QPoint &point);
    void handlePresetClicked(const QAction *_act);
    QStringList extensionsList() const;

    FilterCreation mode_ = FC_Disabled;
    WidgetFileTypes *types_ = nullptr;
    Settings *settings_ = nullptr;
    QString workDir_;

    static const QMap<QString, QStringList> _presets;

protected:
    void keyPressEvent(QKeyEvent* event) override;
}; // class DialogDbCreation

#endif // DIALOGDBCREATION_H
