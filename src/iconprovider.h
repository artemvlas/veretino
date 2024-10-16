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

    template<typename _Enum> // FileStatus or enum Icons
    QIcon cachedIco(const _Enum _value) const
    {
        static QHash<_Enum, QIcon> _cache;

        if (_cache.contains(_value)) {
            return _cache.value(_value);
        }
        else {
            QIcon _ico = QIcon(svgFilePath(_value));
            _cache.insert(_value, _ico);
            return _ico;
        }
    }

    // the cache contains only 64pix sized items
    template<typename _Enum> // FileStatus or enum Icons
    QPixmap cachedPix(const _Enum _value) const
    {
        static QHash<_Enum, QPixmap> _cache;

        if (_cache.contains(_value)) {
            return _cache.value(_value);
        }
        else {
            QPixmap _pix = icon(_value).pixmap(_pix_size); // 64x64
            _cache.insert(_value, _pix);
            return _pix;
        }
    }

}; // class IconProvider

using Icons = IconProvider::Icons;

#endif // ICONPROVIDER_H
