/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "datacontainer.h"
#include <QDebug>
#include <QFile>
#include "treemodeliterator.h"
#include "settings.h"
#include "tools.h"

DataContainer::DataContainer(QObject *parent)
    : QObject(parent) {}

DataContainer::DataContainer(const MetaData &meta, QObject *parent)
    : QObject(parent), metaData(meta) {}

ProxyModel* DataContainer::setProxyModel()
{
    if (proxyModel_)
        delete proxyModel_;

    proxyModel_ = new ProxyModel(model_, this);
    return proxyModel_;
}

QString DataContainer::databaseFileName() const
{
    return paths::basicName(metaData.dbFilePath);
}

QString DataContainer::backupFilePath() const
{
    return paths::joinPath(paths::parentFolder(metaData.dbFilePath),
                           QStringLiteral(u".tmp-backup_") + paths::basicName(metaData.dbFilePath));
}

// returns the absolute path to the db item (file or subfolder)
// (working folder path in filesystem + relative path in the database)
QString DataContainer::itemAbsolutePath(const QModelIndex &curIndex) const
{
    return paths::joinPath(metaData.workDir, TreeModel::getPath(curIndex));
}

// existing = false: returns the predefined path to the branch database file, regardless of the file's existence
// true: returns the path to the existing branch's database file; empty str if missing
QString DataContainer::getBranchFilePath(const QModelIndex &subfolder, bool existing) const
{
    if (!TreeModel::isFolderRow(subfolder))
        return QString();

    //const Settings defaults;
    //QString folderName = subfolder.data().toString();
    QString folderPath = itemAbsolutePath(subfolder);
    //const bool isLongExtension = paths::hasExtension(metaData.dbFilePath, Lit::sl_db_exts.first());

    QString extension = paths::hasExtension(metaData.dbFilePath, Lit::sl_db_exts.at(0)) ? Lit::sl_db_exts.at(0) : Lit::sl_db_exts.at(1);
    QString fileName = format::composeDbFileName(QStringLiteral(u"checksums"), folderPath, extension);
    QString filePath = paths::joinPath(folderPath, fileName);

    if (QFileInfo::exists(filePath) || !existing)
        return filePath;

    /*
    extension = defaults.dbFileExtension(!isLongExtension);
    fileName = format::composeDbFileName(defaults.dbPrefix, folderName, extension);
    filePath = paths::joinPath(folderPath, fileName);

    if (QFileInfo::exists(filePath))
        return filePath;

    return QString();*/

    return Files::firstDbFile(folderPath);
}

QString DataContainer::basicDate() const
{
    // "Created: 2024/09/24 18:35" --> "2024/09/24 18:35"
    const int r_len = Lit::s_dt_format.size(); // 16
    const QString (&dt)[3] = metaData.datetime;

    if (!dt[DTstr::DateVerified].isEmpty())
        return dt[DTstr::DateVerified].right(r_len);

    // if the update time is not empty and there is no verification time,
    // return an empty string

    if (dt[DTstr::DateUpdated].isEmpty())
        return dt[DTstr::DateCreated].right(r_len);

    return QString();
}

bool DataContainer::isWorkDirRelative() const
{
    return (paths::parentFolder(metaData.dbFilePath) == metaData.workDir);
}

bool DataContainer::isFilterApplied() const
{
    return metaData.filter.isFilterEnabled();
}

bool DataContainer::contains(const FileStatuses flags, const QModelIndex &subfolder) const
{
    const Numbers &num = TreeModel::isFolderRow(subfolder) ? getNumbers(subfolder) : numbers;

    return num.contains(flags);
}

bool DataContainer::isAllChecked() const
{
    return (contains(FileStatus::CombChecked)
            && !contains(FileStatus::CombNotChecked | FileStatus::CombProcessing));
}

bool DataContainer::isAllMatched() const
{
    return (!contains(FileStatus::CombProcessing)
            && numbers.numberOf(FileStatus::Matched) == numbers.numberOf(FileStatus::CombHasChecksum));
}

bool DataContainer::isDbFileState(DbFileState state) const
{
    return (state == metaData.dbFileState);
}

bool DataContainer::isInCreation() const
{
    return (metaData.dbFileState == MetaData::NoFile);
}

bool DataContainer::isImmutable() const
{
    return (metaData.flags & MetaData::FlagConst);
}

bool DataContainer::isBackupExists() const
{
    return QFileInfo::exists(backupFilePath());
}

bool DataContainer::makeBackup(bool forceOverwrite) const
{
    if (!QFileInfo::exists(metaData.dbFilePath))
        return false;

    if (forceOverwrite)
        removeBackupFile();

    return QFile::copy(metaData.dbFilePath,
                       backupFilePath());
}

bool DataContainer::restoreBackupFile() const
{
    if (isBackupExists()) {
        if (QFile::exists(metaData.dbFilePath)) {
            if (!QFile::remove(metaData.dbFilePath))
                return false;
        }
        return QFile::rename(backupFilePath(), metaData.dbFilePath);
    }
    return false;
}

void DataContainer::removeBackupFile() const
{
    if (isBackupExists())
        QFile::remove(backupFilePath());
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
    qDebug() << Q_FUNC_INFO << databaseFileName();
}
