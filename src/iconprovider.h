/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef ICONPROVIDER_H
#define ICONPROVIDER_H

#include <QPalette>
#include <QIcon>
#include <QFileIconProvider>
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
        Info,
        NewFile,
        Paste,
        ProcessAbort,
        ProcessStop,
        Save,
        Scan,
        Start,
        Undo,
        Update
    }; // enum Icons

    void setTheme(Theme theme);
    void setTheme(const QPalette &palette);
    Theme theme() const;

    QIcon iconFolder() const;
    QIcon icon(FileStatus status) const;
    QIcon icon(Icons themeIcon) const;
    QIcon icon(const QString &file) const;
    QIcon type_icon(const QString &suffix) const;
    QPixmap pixmap(FileStatus status, int size = 64) const;
    QPixmap pixmap(Icons themeIcon, int size = 64) const;

    static QIcon appIcon();

private:
    bool isDarkTheme(const QPalette &palette) const;
    const QString &themeFolder() const;
    QString svgFilePath(FileStatus status) const;
    QString svgFilePath(Icons icon) const;

    Theme theme_ = Light;
    QFileIconProvider fsIcons;

    static const int _pix_size = 64; // default pixmap size
    static const QString s_folderGeneric;
    static const QString s_folderDark;
    static const QString s_folderLight;
    static const QString s_svg;

    template<typename _Pic = QIcon, typename _Enum> // _Pic(QIcon or QPixmap); _Enum(FileStatus or enum Icons)
    _Pic cached(const _Enum _value) const
    {
        static QHash<_Enum, _Pic> _cache;

        if (_cache.contains(_value)) {
            return _cache.value(_value);
        }
        else {
            _Pic _pic;
            if constexpr(std::is_same_v<_Pic, QIcon>)
                _pic = QIcon(svgFilePath(_value));
            else if constexpr(std::is_same_v<_Pic, QPixmap>)
                _pic = icon(_value).pixmap(_pix_size); // 64x64

            return *_cache.insert(_value, _pic);
        }
    }

}; // class IconProvider

using Icons = IconProvider::Icons;

#endif // ICONPROVIDER_H
