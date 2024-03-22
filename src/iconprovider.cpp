/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "iconprovider.h"

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

QString IconProvider::themeFolder() const
{
    return (theme_ == Dark) ? "dark" : "light";
}

QString IconProvider::svgFilePath(FileStatus status) const
{
    QString iconFileName;

    switch (status) {
    case FileStatus::Queued:
        iconFileName = "queued.svg";
        break;
    case FileStatus::Calculating:
        iconFileName = "processing.svg";
        break;
    case FileStatus::Verifying:
        iconFileName = "processing.svg";
        break;
    case FileStatus::NotChecked:
        iconFileName = "notchecked.svg";
        break;
    case FileStatus::Matched:
        iconFileName = "matched.svg";
        break;
    case FileStatus::Mismatched:
        iconFileName = "mismatched.svg";
        break;
    case FileStatus::New:
        iconFileName = "new.svg";
        break;
    case FileStatus::Missing:
        iconFileName = "missing.svg";
        break;
    case FileStatus::Unreadable:
        iconFileName = "unknown.svg";
        break;
    case FileStatus::Added:
        iconFileName = "added.svg";
        break;
    case Files::Removed:
        iconFileName = "removed.svg";
        break;
    case FileStatus::ChecksumUpdated:
        iconFileName = "update.svg";
        break;
    default:
        iconFileName = "unknown.svg";
        break;
    }

    return ":/icons/color/" + iconFileName;
}

QString IconProvider::svgFilePath(Icons icon) const
{
    QString iconFileName;

    switch (icon) {
    case AddFork:
        iconFileName = "add-fork.svg";
        break;
    case Backup:
        iconFileName = "backup.svg";
        break;
    case Cancel:
        iconFileName = "cancel.svg";
        break;
    case ChartPie:
        iconFileName = "chart-pie.svg";
        break;
    case ClearHistory:
        iconFileName = "clear-history.svg";
        break;
    case Clock:
        iconFileName = "clock.svg";
        break;
    case Configure:
        iconFileName = "configure.svg";
        break;
    case Copy:
        iconFileName = "copy.svg";
        break;
    case Database:
        iconFileName = "database.svg";
        break;
    case DocClose:
        iconFileName = "document-close.svg";
        break;
    case DoubleGear:
        iconFileName = "double-gear.svg";
        break;
    case FileSystem:
        iconFileName = "filesystem.svg";
        break;
    case Filter:
        iconFileName = "filter.svg";
        break;
    case Folder:
        iconFileName = "folder.svg";
        break;
    case FolderSync:
        iconFileName = "folder-sync.svg";
        break;
    case Gear:
        iconFileName = "gear.svg";
        break;
    case GoHome:
        iconFileName = "go-home.svg";
        break;
    case HashFile:
        iconFileName = "hash-file.svg";
        break;
    case NewFile:
        iconFileName = "newfile.svg";
        break;
    case Paste:
        iconFileName = "paste.svg";
        break;
    case ProcessStop:
        iconFileName = "process-stop.svg";
        break;
    case Save:
        iconFileName = "save.svg";
        break;
    case Scan:
        iconFileName = "scan.svg";
        break;
    case Start:
        iconFileName = "start.svg";
        break;
    case Undo:
        iconFileName = "undo.svg";
        break;
    case Update:
        iconFileName = "update.svg";
        break;
    default:
        return QString();
    }

    return QString(":/icons/%1/%2").arg(themeFolder(), iconFileName);
}

QIcon IconProvider::icon(FileStatus status) const
{
    return QIcon(svgFilePath(status));
}

QIcon IconProvider::icon(Icons icon) const
{
    return QIcon(svgFilePath(icon));
}
