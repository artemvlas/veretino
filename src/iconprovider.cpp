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

void IconProvider::setTheme(const QWidget *widget)
{
    setTheme(widget->palette());
}

void IconProvider::setTheme(const QPalette &palette)
{
    theme_ = isDarkTheme(palette) ? Dark : Light;
}

bool IconProvider::isDarkTheme(const QPalette &palette)
{
    int curLightness = palette.color(QPalette::Active, QPalette::Base).lightness();
    return (curLightness < 120);
}

QIcon IconProvider::icon(FileStatus status) const
{
    switch (status) {
    case FileStatus::Queued:
        return QIcon(":/icons/color/queued.svg");
    case FileStatus::Calculating:
        return QIcon(":/icons/color/processing.svg");
    case FileStatus::Verifying:
        return QIcon(":/icons/color/processing.svg");
    case FileStatus::NotChecked:
        return QIcon(":/icons/color/notchecked.svg");
    case FileStatus::Matched:
        return QIcon(":/icons/color/matched.svg");
    case FileStatus::Mismatched:
        return QIcon(":/icons/color/mismatched.svg");
    case FileStatus::New:
        return QIcon(":/icons/color/new.svg");
    case FileStatus::Missing:
        return QIcon(":/icons/color/missing.svg");
    case FileStatus::Unreadable:
        return QIcon(":/icons/color/unknown.svg");
    case FileStatus::Added:
        return QIcon(":/icons/color/added.svg");
    case Files::Removed:
        return QIcon(":/icons/color/removed.svg");
    case FileStatus::ChecksumUpdated:
        return QIcon(":/icons/color/update.svg");
    default:
        return QIcon(":/icons/color/unknown.svg");
    }
}

QIcon IconProvider::icon(Icons icon) const
{
    QString theme = (theme_ == Dark) ? "dark" : "light";

    switch (icon) {
    case AddFork:
        return QIcon(QString(":/icons/%1/add-fork.svg").arg(theme));
    case Backup:
        return QIcon(QString(":/icons/%1/backup.svg").arg(theme));
    case Cancel:
        return QIcon(QString(":/icons/%1/cancel.svg").arg(theme));
    case ChartPie:
        return QIcon(QString(":/icons/%1/chart-pie.svg").arg(theme));
    case ClearHistory:
        return QIcon(QString(":/icons/%1/clear-history.svg").arg(theme));
    case Clock:
        return QIcon(QString(":/icons/%1/clock.svg").arg(theme));
    case Configure:
        return QIcon(QString(":/icons/%1/configure.svg").arg(theme));
    case Copy:
        return QIcon(QString(":/icons/%1/copy.svg").arg(theme));
    case Database:
        return QIcon(QString(":/icons/%1/database.svg").arg(theme));
    case DocClose:
        return QIcon(QString(":/icons/%1/document-close.svg").arg(theme));
    case DoubleGear:
        return QIcon(QString(":/icons/%1/double-gear.svg").arg(theme));
    case FileSystem:
        return QIcon(QString(":/icons/%1/filesystem.svg").arg(theme));
    case Filter:
        return QIcon(QString(":/icons/%1/filter.svg").arg(theme));
    case Folder:
        return QIcon(QString(":/icons/%1/folder.svg").arg(theme));
    case FolderSync:
        return QIcon(QString(":/icons/%1/folder-sync.svg").arg(theme));
    case Gear:
        return QIcon(QString(":/icons/%1/gear.svg").arg(theme));
    case GoHome:
        return QIcon(QString(":/icons/%1/go-home.svg").arg(theme));
    case NewFile:
        return QIcon(QString(":/icons/%1/newfile.svg").arg(theme));
    case Paste:
        return QIcon(QString(":/icons/%1/paste.svg").arg(theme));
    case ProcessStop:
        return QIcon(QString(":/icons/%1/process-stop.svg").arg(theme));
    case Save:
        return QIcon(QString(":/icons/%1/save.svg").arg(theme));
    case Scan:
        return QIcon(QString(":/icons/%1/scan.svg").arg(theme));
    case Start:
        return QIcon(QString(":/icons/%1/start.svg").arg(theme));
    case Undo:
        return QIcon(QString(":/icons/%1/undo.svg").arg(theme));
    case Update:
        return QIcon(QString(":/icons/%1/update.svg").arg(theme));
    default:
        return QIcon();
    }
}
