/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "datacontainer.h"
#include <QDebug>
#include <QFile>
#include <QStandardPaths>
#include "treemodeliterator.h"

DataContainer::DataContainer(QObject *parent)
    : QObject(parent) {}

ProxyModel* DataContainer::setProxyModel()
{
    if (proxyModel_)
        delete proxyModel_;

    proxyModel_ = new ProxyModel(model_, this);
    return proxyModel_;
}

QString DataContainer::databaseFileName() const
{
    return paths::basicName(metaData.databaseFilePath);
}

QString DataContainer::backupFilePath() const
{
    return paths::joinPath(paths::parentFolder(metaData.databaseFilePath),
                           ".tmp-backup_" + paths::basicName(metaData.databaseFilePath));
}

// returns the absolute path to the database subfolder
// (working folder path in filesystem + relative path in the database)
QString DataContainer::dbSubFolderAbsolutePath(const QModelIndex &subfolder) const
{
    return paths::joinPath(metaData.workDir, TreeModel::getPath(subfolder));
}

QString DataContainer::dbSubFolderDbFilePath(const QModelIndex &subfolder) const
{
    if (!subfolder.isValid())
        return QString();

    const Settings defaults;
    QString extension = defaults.dbFileExtension(!metaData.databaseFilePath.endsWith(".ver"));
    QString subFolderDbFileName = format::composeDbFileName(defaults.dbPrefix, subfolder.data().toString(), extension);

    return paths::joinPath(dbSubFolderAbsolutePath(subfolder), subFolderDbFileName);
}

bool DataContainer::isWorkDirRelative() const
{
    return (paths::parentFolder(metaData.databaseFilePath) == metaData.workDir);
}

bool DataContainer::isFilterApplied() const
{
    return !metaData.filter.extensionsList.isEmpty();
}

bool DataContainer::contains(const FileStatus status) const
{
    return (numbers.numberOf(status) > 0);
}

bool DataContainer::contains(const FileStatusFlags flags) const
{
    return (flags == FlagsAvailable) ? (numbers.available() > 0)
                                     : contains(flagsList(flags));
}

bool DataContainer::contains(const QList<FileStatus> &statuses) const
{
    QList<FileStatus>::const_iterator it;
    for (it = statuses.constBegin(); it != statuses.constEnd(); ++it) {
        if (contains(*it))
            return true;
    }

    return false;
}

bool DataContainer::isAllChecked() const
{
    return (contains(FileStatusFlags::FlagsChecked) && !contains(FileStatus::NotChecked));
}

bool DataContainer::isBackupExists()
{
    return (QFile::exists(backupFilePath()));
}

bool DataContainer::makeBackup(bool forceOverwrite)
{
    if (!QFile::exists(metaData.databaseFilePath))
        return false;

    if (forceOverwrite)
        removeBackupFile();

    return QFile::copy(metaData.databaseFilePath,
                       backupFilePath());
}

bool DataContainer::restoreBackupFile()
{
    if (isBackupExists()) {
        if (QFile::exists(metaData.databaseFilePath)) {
            if (!QFile::remove(metaData.databaseFilePath))
                return false;
        }
        return QFile::rename(backupFilePath(), metaData.databaseFilePath);
    }
    return false;
}

void DataContainer::removeBackupFile()
{
    if (isBackupExists())
        QFile::remove(backupFilePath());
}

void DataContainer::setSaveResult(const QString &dbFilePath)
{
    if (!tools::isDatabaseFile(dbFilePath))
        metaData.saveResult = MetaData::NotSaved;
    else if (paths::parentFolder(dbFilePath) == QStandardPaths::writableLocation(QStandardPaths::DesktopLocation))
        metaData.saveResult = MetaData::SavedToDesktop;
    else
        metaData.saveResult = MetaData::Saved;

    if (metaData.saveResult == MetaData::Saved || metaData.saveResult == MetaData::SavedToDesktop)
        metaData.databaseFilePath = dbFilePath;
}

const Numbers& DataContainer::updateNumbers()
{
    numbers = getNumbers();
    return numbers;
}

Numbers DataContainer::getNumbers(const QModelIndex &rootIndex)
{
    return getNumbers(model_, rootIndex);
}

Numbers DataContainer::getNumbers(const QAbstractItemModel *model, const QModelIndex &rootIndex)
{
    Numbers num;

    TreeModelIterator iter(model, rootIndex);

    while (iter.hasNext()) {
        iter.nextFile();

        if (iter.data(Column::ColumnChecksum).isValid()
            && !iter.data(Column::ColumnChecksum).toString().isEmpty()) {

            ++num.numChecksums;
            num.totalSize += iter.data(Column::ColumnSize).toLongLong();
        }

        if (!num.holder.contains(iter.status())) {
            num.holder.insert(iter.status(), 1);
        }
        else {
            int storedNumber = num.holder.value(iter.status());
            num.holder.insert(iter.status(), ++storedNumber);
        }
    }

    return num;
}

const QList<FileStatus>& DataContainer::flagsList(FileStatusFlags flags) const
{
    switch (flags) {
    case FlagsAvailable:
        return flagsAvailable;
    case FlagsUpdatable:
        return flagsUpdatable;
    case FlagsChecked:
        return flagsChecked;
    default:
        return flagsEmpty;
    }
}

DataContainer::~DataContainer()
{
    removeBackupFile();
    qDebug() << "DataContainer deleted" << databaseFileName();
}
