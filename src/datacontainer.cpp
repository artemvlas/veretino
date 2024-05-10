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

DataContainer::DataContainer(const MetaData &metadata, QObject *parent)
    : QObject(parent), metaData(metadata) {}

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

// returns the absolute path to the database item (file or subfolder)
// (working folder path in filesystem + relative path in the database)
QString DataContainer::itemAbsolutePath(const QModelIndex &curIndex) const
{
    return paths::joinPath(metaData.workDir, TreeModel::getPath(curIndex));
}

QString DataContainer::getBranchFilePath(const QModelIndex &subfolder, bool existing) const
{
    if (!TreeModel::isFolderRow(subfolder))
        return QString();

    const Settings defaults;
    QString folderName = subfolder.data().toString();
    QString folderPath = itemAbsolutePath(subfolder);
    const bool isLongExtension = !metaData.databaseFilePath.endsWith(".ver");

    QString extension = defaults.dbFileExtension(isLongExtension);
    QString fileName = format::composeDbFileName(defaults.dbPrefix, folderName, extension);
    QString filePath = paths::joinPath(folderPath, fileName);

    if (QFile::exists(filePath) || !existing)
        return filePath;

    extension = defaults.dbFileExtension(!isLongExtension);
    fileName = format::composeDbFileName(defaults.dbPrefix, folderName, extension);
    filePath = paths::joinPath(folderPath, fileName);

    if (QFile::exists(filePath))
        return filePath;

    return QString();
}

bool DataContainer::isWorkDirRelative() const
{
    return (paths::parentFolder(metaData.databaseFilePath) == metaData.workDir);
}

bool DataContainer::isFilterApplied() const
{
    return !metaData.filter.extensionsList.isEmpty();
}

bool DataContainer::contains(const FileStatuses flags, const QModelIndex &subfolder) const
{
    const Numbers &num = TreeModel::isFolderRow(subfolder) ? getNumbers(subfolder) : numbers;

    return num.contains(flags);
}

bool DataContainer::isAllChecked() const
{
    return (contains(FileStatus::FlagChecked) && !contains(FileStatus::NotChecked));
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

Numbers DataContainer::getNumbers(const QModelIndex &rootIndex) const
{
    return getNumbers(model_, rootIndex);
}

Numbers DataContainer::getNumbers(const QAbstractItemModel *model, const QModelIndex &rootIndex)
{
    Numbers num;

    TreeModelIterator iter(model, rootIndex);

    while (iter.hasNext()) {
        iter.nextFile();
        num.addFile(iter.status(), iter.size());
    }

    return num;
}

DataContainer::~DataContainer()
{
    removeBackupFile();
    qDebug() << "DataContainer deleted" << databaseFileName();
}
