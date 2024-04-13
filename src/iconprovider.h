/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
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

    enum Theme {Light, Dark};
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

    QIcon icon(FileStatus status) const;
    QIcon icon(Icons icon) const;
    static QIcon iconVeretino();

private:
    bool isDarkTheme(const QPalette &palette) const;
    QString themeFolder() const;
    QString svgFilePath(FileStatus status) const;
    QString svgFilePath(Icons icon) const;

    Theme theme_ = Light;
};

using Icons = IconProvider::Icons;

#endif // ICONPROVIDER_H
