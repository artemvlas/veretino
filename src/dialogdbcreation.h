/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DIALOGDBCREATION_H
#define DIALOGDBCREATION_H

#include <QDialog>
#include <QKeyEvent>
#include <QCheckBox>
#include "iconprovider.h"
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
    bool isFilterCreating() const;
    QString getComment() const;

private:
    Ui::DialogDbCreation *ui;
    QCheckBox *cb_file_filter = new QCheckBox(this);

    void connections();
    void setDbConfig();
    void setFilterConfig();
    void restoreLastExts();
    void parseInputedExts();
    void resetView();
    void updateDbFilename();
    void setTotalInfo(const FileTypeList &exts);
    void setItemsVisibility(bool isTop10Checked);
    void setCheckboxesVisible(bool visible);
    void clearChecked();
    void updateViewMode();
    void activateItem(QTreeWidgetItem *tItem);
    void updateFilterDisplay();
    void updateLabelFilterExtensions();
    void updateLabelTotalFiltered();
    void createMenuWidgetTypes(const QPoint &point);
    void handlePresetClicked(const QAction *act);
    int cmbAlgoIndex();
    QIcon presetIcon(const QString &name) const;
    QStringList inputedExts() const;
    FilterMode curFilterMode() const;

    FilterCreation m_mode = FC_Disabled;
    WidgetFileTypes *m_types = nullptr;
    Settings *m_settings = nullptr;
    QString m_workDir;
    IconProvider m_icons;

    static const QMap<QString, QSet<QString>> s_presets;

protected:
    void keyPressEvent(QKeyEvent* event) override;
}; // class DialogDbCreation

#endif // DIALOGDBCREATION_H
