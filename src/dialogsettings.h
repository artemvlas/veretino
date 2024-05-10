/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef DIALOGSETTINGS_H
#define DIALOGSETTINGS_H

#include <QDialog>
#include <QVariant>
#include "tools.h"

namespace Ui {
class DialogSettings;
}

class DialogSettings : public QDialog
{
    Q_OBJECT

public:
    explicit DialogSettings(Settings *settings, QWidget *parent = nullptr);
    ~DialogSettings();
    void updateSettings();
    enum Tabs {TabMain, TabDatabase, TabFilter};

private:
    enum FilterPreset {PresetCustom, PresetDocuments, PresetPictures, PresetMusic, PresetVideos, PresetIgnoreTriflings};

    void loadSettings(const Settings &settings);
    void restoreDefaults();
    void updateLabelDatabaseFilename();
    void setExtensionsColor();
    void setComboBoxFpIndex();
    void setComboBoxFpIndex(const FilterRule &filter);
    void setFilterPreset(int presetIndex);
    void setFilterRule(const FilterRule &filter);
    void handleFilterMode();
    void cleanUpExtList();

    FilterRule selectPresetFilter(const FilterPreset preset);
    FilterRule getCurrentFilter() const;

    Ui::DialogSettings *ui;
    QStringList extensionsList() const; // return a list of extensions from input
    Settings *settings_;
    const Settings defaults;

    const QStringList filterPresetsList = {"Custom...", "Documents", "Pictures", "Music", "Videos", "Ignore Triflings"};
    const QStringList listPresetDocuments = {"odt", "ods", "pdf", "docx", "xlsx", "doc", "rtf", "txt"};
    const QStringList listPresetPictures = {"jpg", "jpeg", "png", "svg", "webp"};
    const QStringList listPresetMusic = {"flac", "wv", "ape", "oga", "ogg", "opus", "m4a", "mp3"};
    const QStringList listPresetVideos = {"mkv", "webm", "mp4", "m4v", "avi"};
    const QStringList listPresetIgnoreTriflings = {"log", "cue", "txt"};
};

#endif // DIALOGSETTINGS_H
