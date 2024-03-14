#ifndef ICONPROVIDER_H
#define ICONPROVIDER_H
#include <QWidget>
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
    void setTheme(const QWidget *widget);
    void setTheme(const QPalette &palette);

    QIcon icon(FileStatus status) const;
    QIcon icon(Icons icon) const;

private:
    bool isDarkTheme(const QPalette &palette);
    Theme theme_ = Light;
};

using Icons = IconProvider::Icons;

#endif // ICONPROVIDER_H
