/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef ICONPROVIDER_H
#define ICONPROVIDER_H
#include <QPalette>
#include <QIcon>
#include "files.h"

class IconProvider
{
public:
    IconProvider();
    IconProvider(const QPalette &palette);

    enum Theme { Light, Dark };
    enum Icons {
        AddFork,
        Backup,
        Branch,
        Cancel,
        ChartPie,
        ClearHistory,
        Clock,
        Configure,
        Copy,
        Database,
        DocClose,
        DoubleGear,
        FileSystem,
        Filter,
        Folder,
        FolderSync,
        Gear,
        GoHome,
        HashFile,
        NewFile,
        Paste,
        ProcessStop,
        Save,
        Scan,
        Start,
        Undo,
        Update
    };

    void setTheme(Theme theme);
    void setTheme(const QPalette &palette);
    Theme theme() const;

    QIcon icon(FileStatus status) const;
    QIcon icon(Icons themeIcon) const;
    static QIcon appIcon();

private:
    bool isDarkTheme(const QPalette &palette) const;
    QString themeFolder() const;
    QString svgFilePath(FileStatus status) const;
    QString svgFilePath(Icons icon) const;

    static QHash<FileStatus, QIcon> cacheFileStatus;
    static QHash<Icons, QIcon> cacheThemeIcons;
    Theme theme_ = Light;

}; // class IconProvider

using Icons = IconProvider::Icons;

#endif // ICONPROVIDER_H
