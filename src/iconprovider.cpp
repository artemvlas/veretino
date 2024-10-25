/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "iconprovider.h"
#include "tools.h"

const QString IconProvider::s_folderGeneric = QStringLiteral(u":/icons/generic");
const QString IconProvider::s_folderDark = QStringLiteral(u":/icons/dark");
const QString IconProvider::s_folderLight = QStringLiteral(u":/icons/light");
const QString IconProvider::s_svg = QStringLiteral(u"svg");

IconProvider::IconProvider() {}

IconProvider::IconProvider(const QPalette &palette)
{
    setTheme(palette);
}

void IconProvider::setTheme(Theme theme)
{
    theme_ = theme;
}

void IconProvider::setTheme(const QPalette &palette)
{
    theme_ = isDarkTheme(palette) ? Dark : Light;
}

bool IconProvider::isDarkTheme(const QPalette &palette) const
{
    int curLightness = palette.color(QPalette::Active, QPalette::Base).lightness();
    return (curLightness < 120);
}

IconProvider::Theme IconProvider::theme() const
{
    return theme_;
}

const QString &IconProvider::themeFolder() const
{
    return (theme_ == Dark) ? s_folderDark : s_folderLight;
}

QString IconProvider::svgFilePath(FileStatus status) const
{
    QString iconFileName;

    switch (status) {
    case FileStatus::Queued:
        iconFileName = QStringLiteral(u"queued");
        break;
    case FileStatus::Calculating:
        iconFileName = QStringLiteral(u"processing");
        break;
    case FileStatus::Verifying:
        iconFileName = QStringLiteral(u"processing");
        break;
    case FileStatus::NotChecked:
        iconFileName = QStringLiteral(u"notchecked");
        break;
    case FileStatus::NotCheckedMod:
        iconFileName = QStringLiteral(u"outdated");
        break;
    case FileStatus::Matched:
        iconFileName = QStringLiteral(u"matched");
        break;
    case FileStatus::Mismatched:
        iconFileName = QStringLiteral(u"mismatched");
        break;
    case FileStatus::New:
        iconFileName = QStringLiteral(u"new");
        break;
    case FileStatus::Missing:
        iconFileName = QStringLiteral(u"missing");
        break;
    case FileStatus::Added:
        iconFileName = QStringLiteral(u"added");
        break;
    case FileStatus::Removed:
        iconFileName = QStringLiteral(u"removed");
        break;
    case FileStatus::Updated:
        iconFileName = QStringLiteral(u"update");
        break;
    case FileStatus::Moved:
    case FileStatus::MovedOut:
        iconFileName = QStringLiteral(u"moved");
        break;
    case FileStatus::UnPermitted:
        iconFileName = QStringLiteral(u"locked");
        break;
    case FileStatus::ReadError:
        iconFileName = QStringLiteral(u"unreadable");
        break;
    default:
        iconFileName = QStringLiteral(u"unknown");
        break;
    }

    return paths::composeFilePath(s_folderGeneric, iconFileName, s_svg);
}

QString IconProvider::svgFilePath(Icons icon) const
{
    QString iconFileName;

    switch (icon) {
    case AddFork:
        iconFileName = QStringLiteral(u"add-fork");
        break;
    case Backup:
        iconFileName = QStringLiteral(u"backup");
        break;
    case Branch:
        iconFileName = QStringLiteral(u"branch");
        break;
    case Cancel:
        iconFileName = QStringLiteral(u"cancel");
        break;
    case ChartPie:
        iconFileName = QStringLiteral(u"chart-pie");
        break;
    case ClearHistory:
        iconFileName = QStringLiteral(u"clear-history");
        break;
    case Clock:
        iconFileName = QStringLiteral(u"clock");
        break;
    case Configure:
        iconFileName = QStringLiteral(u"configure");
        break;
    case Copy:
        iconFileName = QStringLiteral(u"copy");
        break;
    case Database:
        iconFileName = QStringLiteral(u"database");
        break;
    case DocClose:
        iconFileName = QStringLiteral(u"document-close");
        break;
    case DoubleGear:
        iconFileName = QStringLiteral(u"double-gear");
        break;
    case FileSystem:
        iconFileName = QStringLiteral(u"filesystem");
        break;
    case Filter:
        iconFileName = QStringLiteral(u"filter");
        break;
    case Folder:
        iconFileName = QStringLiteral(u"folder");
        break;
    case FolderSync:
        iconFileName = QStringLiteral(u"folder-sync");
        break;
    case Gear:
        iconFileName = QStringLiteral(u"gear");
        break;
    case GoHome:
        iconFileName = QStringLiteral(u"go-home");
        break;
    case HashFile:
        iconFileName = QStringLiteral(u"hash-file");
        break;
    case Info:
        iconFileName = QStringLiteral(u"help-about");
        break;
    case NewFile:
        iconFileName = QStringLiteral(u"newfile");
        break;
    case Paste:
        iconFileName = QStringLiteral(u"paste");
        break;
    case ProcessAbort:
        iconFileName = QStringLiteral(u"process-abort");
        break;
    case ProcessStop:
        iconFileName = QStringLiteral(u"process-stop");
        break;
    case Save:
        iconFileName = QStringLiteral(u"save");
        break;
    case Scan:
        iconFileName = QStringLiteral(u"scan");
        break;
    case Start:
        iconFileName = QStringLiteral(u"start");
        break;
    case Undo:
        iconFileName = QStringLiteral(u"undo");
        break;
    case Update:
        iconFileName = QStringLiteral(u"update");
        break;
    default:
        return QString();
    }

    return paths::composeFilePath(themeFolder(), iconFileName, s_svg);
}

QIcon IconProvider::icon(FileStatus status) const
{
    return cached(status);
}

QIcon IconProvider::icon(Icons themeIcon) const
{
    return cached(themeIcon);
}

QIcon IconProvider::icon(const QString &file) const
{
    return fsIcons.icon(QFileInfo(file));
}

QIcon IconProvider::type_icon(const QString &suffix) const
{
    return icon(QStringLiteral(u"file.") + suffix);
}

QIcon IconProvider::iconFolder() const
{
    return fsIcons.icon(QFileIconProvider::Folder);
}

QIcon IconProvider::appIcon()
{
    static const QIcon _icon = QIcon(paths::composeFilePath(s_folderGeneric, Lit::s_app_name, s_svg));
    return _icon; // ":/icons/generic/veretino.svg"
}

QPixmap IconProvider::pixmap(FileStatus status, int size) const
{
    if (size != _pix_size)
        return icon(status).pixmap(size);

    return cached<QPixmap>(status);
}

QPixmap IconProvider::pixmap(Icons themeIcon, int size) const
{
    if (size != _pix_size)
        return icon(themeIcon).pixmap(size);

    return cached<QPixmap>(themeIcon);
}
