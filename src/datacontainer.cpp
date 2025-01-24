/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "datacontainer.h"
#include <QDebug>
#include <QFileInfo>
#include "treemodeliterator.h"
#include "tools.h"
#include "pathstr.h"

DataContainer::DataContainer(QObject *parent)
    : QObject(parent) {}

DataContainer::DataContainer(const MetaData &meta, QObject *parent)
    : QObject(parent), metaData_(meta) {}

ProxyModel* DataContainer::setProxyModel()
{
    if (p_proxy)
        delete p_proxy;

    p_proxy = new ProxyModel(p_model, this);
    return p_proxy;
}

QString DataContainer::databaseFileName() const
{
    return pathstr::basicName(metaData_.dbFilePath);
}

QString DataContainer::backupFilePath() const
{
    using namespace pathstr;
    return joinPath(parentFolder(metaData_.dbFilePath),
                    QStringLiteral(u".tmp-backup_") + basicName(metaData_.dbFilePath));
}

// returns the absolute path to the db item (file or subfolder)
// (working folder path in filesystem + relative path in the database)
QString DataContainer::itemAbsolutePath(const QModelIndex &curIndex) const
{
    return pathstr::joinPath(metaData_.workDir, TreeModel::getPath(curIndex));
}

QString DataContainer::branch_path_existing(const QModelIndex &subfolder)
{
    if (!TreeModel::isFolderRow(subfolder))
        return QString();

    if (_cacheBranches.contains(subfolder)) {
        const QString __fp = _cacheBranches.value(subfolder);
        if (__fp.isEmpty() || QFileInfo::exists(__fp)) { // in case of renaming
            qDebug() << "DC::branchFilePath >> return cached:" << __fp;
            return __fp;
        }
    }

    // searching
    QString __path = branch_path_composed(subfolder);
    if (!QFileInfo::exists(__path))
        __path = Files::firstDbFile(pathstr::parentFolder(__path));

    // caching the result; an empty value means no branches
    _cacheBranches[subfolder] = __path;

    return __path;
}

// returns the predefined/supposed path to the branch db file, regardless of its existence
QString DataContainer::branch_path_composed(const QModelIndex &subfolder) const
{
    const bool _isLongExt = pathstr::hasExtension(metaData_.dbFilePath, Lit::sl_db_exts.first());
    QString extension = Lit::sl_db_exts.at(_isLongExt ? 0 : 1);
    QString folderPath = itemAbsolutePath(subfolder);
    QString fileName = format::composeDbFileName(QStringLiteral(u"checksums"), folderPath, extension);
    QString filePath = pathstr::joinPath(folderPath, fileName);

    return filePath;
}

QString DataContainer::digestFilePath(const QModelIndex &fileIndex) const
{
    return paths::digestFilePath(itemAbsolutePath(fileIndex), metaData_.algorithm);
}

bool DataContainer::isWorkDirRelative() const
{
    return (pathstr::parentFolder(metaData_.dbFilePath) == metaData_.workDir);
}

bool DataContainer::isFilterApplied() const
{
    return metaData_.filter.isEnabled();
}

bool DataContainer::contains(const FileStatuses flags, const QModelIndex &subfolder) const
{
    const Numbers &num = TreeModel::isFolderRow(subfolder) ? getNumbers(subfolder) : m_numbers;

    return num.contains(flags);
}

bool DataContainer::isAllChecked() const
{
    return (contains(FileStatus::CombChecked)
            && !contains(FileStatus::CombNotChecked | FileStatus::CombProcessing));
}

bool DataContainer::isAllMatched(const QModelIndex &subfolder) const
{
    const Numbers &_nums = TreeModel::isFolderRow(subfolder) ? getNumbers(subfolder) : m_numbers;

    return isAllMatched(_nums);
}

bool DataContainer::isAllMatched(const Numbers &nums) const
{
    return (!nums.contains(FileStatus::CombProcessing)
            && nums.numberOf(FileStatus::Matched) == nums.numberOf(FileStatus::CombHasChecksum));
}

bool DataContainer::isDbFileState(DbFileState state) const
{
    return (state == metaData_.dbFileState);
}

bool DataContainer::isInCreation() const
{
    return (metaData_.dbFileState == MetaData::NoFile);
}

bool DataContainer::isImmutable() const
{
    return (metaData_.flags & MetaData::FlagConst);
}

bool DataContainer::hasPossiblyMovedItems() const
{
    return (contains(FileStatus::New) && contains(FileStatus::Missing));
}

bool DataContainer::isBackupExists() const
{
    return QFileInfo::exists(backupFilePath());
}

bool DataContainer::makeBackup(bool forceOverwrite) const
{
    if (!QFileInfo::exists(metaData_.dbFilePath))
        return false;

    if (forceOverwrite)
        removeBackupFile();

    return QFile::copy(metaData_.dbFilePath,
                       backupFilePath());
}

bool DataContainer::restoreBackupFile() const
{
    if (isBackupExists()) {
        if (QFile::exists(metaData_.dbFilePath)) {
            if (!QFile::remove(metaData_.dbFilePath))
                return false;
        }
        return QFile::rename(backupFilePath(), metaData_.dbFilePath);
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
    m_numbers = getNumbers();
    return m_numbers;
}

Numbers DataContainer::getNumbers(const QModelIndex &rootIndex) const
{
    return getNumbers(p_model, rootIndex);
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

void DataContainer::clearData()
{
    if (p_proxy)
        delete p_proxy;
    if (p_model)
        delete p_model;

    p_model = new TreeModel(this);
    p_proxy = new ProxyModel(p_model, this);

    m_numbers.clear();
}

DataContainer::~DataContainer()
{
    removeBackupFile();
    qDebug() << Q_FUNC_INFO << databaseFileName();
}
