/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "iconprovider.h"

QHash<FileStatus, QIcon> IconProvider::cacheFileStatus = QHash<FileStatus, QIcon>();
QHash<Icons, QIcon> IconProvider::cacheThemeIcons = QHash<Icons, QIcon>();

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

QString IconProvider::themeFolder() const
{
    return (theme_ == Dark) ? "dark" : "light";
}

QString IconProvider::svgFilePath(FileStatus status) const
{
    QString iconFileName;

    switch (status) {
    case FileStatus::Queued:
        iconFileName = "queued";
        break;
    case FileStatus::Calculating:
        iconFileName = "processing";
        break;
    case FileStatus::Verifying:
        iconFileName = "processing";
        break;
    case FileStatus::NotChecked:
        iconFileName = "notchecked";
        break;
    case FileStatus::Matched:
        iconFileName = "matched";
        break;
    case FileStatus::Mismatched:
        iconFileName = "mismatched";
        break;
    case FileStatus::New:
        iconFileName = "new";
        break;
    case FileStatus::Missing:
        iconFileName = "missing";
        break;
    case FileStatus::Unreadable:
        iconFileName = "unreadable";
        break;
    case FileStatus::Added:
        iconFileName = "added";
        break;
    case Files::Removed:
        iconFileName = "removed";
        break;
    case FileStatus::Updated:
        iconFileName = "update";
        break;
    case FileStatus::UnPermitted: // newly added status
        iconFileName = "locked";
        break;
    default:
        iconFileName = "unknown";
        break;
    }

    return QString(":/icons/generic/%1.svg").arg(iconFileName);
}

QString IconProvider::svgFilePath(Icons icon) const
{
    QString iconFileName;

    switch (icon) {
    case AddFork:
        iconFileName = "add-fork";
        break;
    case Backup:
        iconFileName = "backup";
        break;
    case Branch:
        iconFileName = "branch";
        break;
    case Cancel:
        iconFileName = "cancel";
        break;
    case ChartPie:
        iconFileName = "chart-pie";
        break;
    case ClearHistory:
        iconFileName = "clear-history";
        break;
    case Clock:
        iconFileName = "clock";
        break;
    case Configure:
        iconFileName = "configure";
        break;
    case Copy:
        iconFileName = "copy";
        break;
    case Database:
        iconFileName = "database";
        break;
    case DocClose:
        iconFileName = "document-close";
        break;
    case DoubleGear:
        iconFileName = "double-gear";
        break;
    case FileSystem:
        iconFileName = "filesystem";
        break;
    case Filter:
        iconFileName = "filter";
        break;
    case Folder:
        iconFileName = "folder";
        break;
    case FolderSync:
        iconFileName = "folder-sync";
        break;
    case Gear:
        iconFileName = "gear";
        break;
    case GoHome:
        iconFileName = "go-home";
        break;
    case HashFile:
        iconFileName = "hash-file";
        break;
    case Info:
        iconFileName = "help-about";
        break;
    case NewFile:
        iconFileName = "newfile";
        break;
    case Paste:
        iconFileName = "paste";
        break;
    case ProcessAbort:
        iconFileName = "process-abort";
        break;
    case ProcessStop:
        iconFileName = "process-stop";
        break;
    case Save:
        iconFileName = "save";
        break;
    case Scan:
        iconFileName = "scan";
        break;
    case Start:
        iconFileName = "start";
        break;
    case Undo:
        iconFileName = "undo";
        break;
    case Update:
        iconFileName = "update";
        break;
    default:
        return QString();
    }

    return QString(":/icons/%1/%2.svg").arg(themeFolder(), iconFileName);
}

QIcon IconProvider::icon(FileStatus status) const
{
    if (cacheFileStatus.contains(status)) {
        return cacheFileStatus.value(status);
    }
    else {
        QIcon ico = QIcon(svgFilePath(status));
        cacheFileStatus.insert(status, ico);
        return ico;
    }
}

QIcon IconProvider::icon(Icons themeIcon) const
{
    if (cacheThemeIcons.contains(themeIcon)) {
        return cacheThemeIcons.value(themeIcon);
    }
    else {
        QIcon ico = QIcon(svgFilePath(themeIcon));
        cacheThemeIcons.insert(themeIcon, ico);
        return ico;
    }
}

QIcon IconProvider::icon(const QString &file) const
{
    return fsIcons.icon(QFileInfo(file));
}

QIcon IconProvider::iconFolder() const
{
    return fsIcons.icon(QFileIconProvider::Folder);
}

QIcon IconProvider::appIcon()
{
    static const QIcon iconVeretino = QIcon(":/icons/generic/veretino.svg");
    return iconVeretino;
}
