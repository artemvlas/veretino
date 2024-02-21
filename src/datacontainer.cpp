/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "datacontainer.h"
#include <QDebug>
#include <QFile>
#include <QStandardPaths>

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

    QString extension = metaData.databaseFilePath.endsWith(".ver") ? ".ver" : ".ver.json";
    QString subFolderDbFileName = format::composeDbFileName("checksums", subfolder.data().toString(), extension);

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
    return (numbers.numberOf(status) != 0);
}

bool DataContainer::contains(const QSet<FileStatus> statuses) const
{
    QSet<FileStatus>::const_iterator it;
    for (it = statuses.constBegin(); it != statuses.constEnd(); ++it) {
        if (contains(*it))
            return true;
    }

    return false;
}

bool DataContainer::containsChecked() const
{
    return (contains(FileStatus::Matched) || contains(FileStatus::Mismatched));
}

bool DataContainer::isAllChecked() const
{
    return (containsChecked() && !contains(FileStatus::NotChecked));
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

DataContainer::~DataContainer()
{
    removeBackupFile();
    qDebug() << "DataContainer deleted" << databaseFileName();
}
