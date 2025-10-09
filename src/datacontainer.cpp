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
#include "files.h"

DataContainer::DataContainer(QObject *parent)
    : QObject(parent) {}

DataContainer::DataContainer(const MetaData &meta, TreeModel *data, QObject *parent)
    : m_metadata(meta), m_model(data), QObject(parent)
{
    if (m_model)
        m_model->setParent(this);
}

DataContainer::~DataContainer()
{
    DataHelper::removeBackupFile(this);
    qDebug() << Q_FUNC_INFO << DataHelper::databaseFileName(this);
}

void DataContainer::set(const MetaData &meta, TreeModel *data)
{
    if (!data)
        return;

    if (m_proxy) {
        delete m_proxy;
        m_proxy = nullptr;
    }

    if (m_model) {
        delete m_model;
        m_model = nullptr;
    }

    m_model = data;
    m_model->setParent(this);
    m_proxy = new ProxyModel(m_model, this);

    m_metadata = meta;

    m_numbers.clear();
    m_cacheMissing.clear();
    m_cacheBranches.clear();
}

void DataContainer::clear()
{
    set(MetaData(), new TreeModel(this));
}

bool DataContainer::isEmpty() const
{
    return !m_model || m_model->isEmpty();
}

/*** <!!!> ***/
/*** DataHelper is a TEMPORARY holder of functions separated from the DataContainer ***/
/*** They will be moved or changed in the future ***/
QString DataHelper::databaseFileName(const DataContainer *data)
{
    return pathstr::basicName(data->m_metadata.dbFilePath);
}

QString DataHelper::backupFilePath(const DataContainer *data)
{
    using namespace pathstr;
    return joinPath(parentFolder(data->m_metadata.dbFilePath),
                    QStringLiteral(u".tmp-backup_") + basicName(data->m_metadata.dbFilePath));
}

// returns the absolute path to the db item (file or subfolder)
// (working folder path in filesystem + relative path in the database)
QString DataHelper::itemAbsolutePath(const DataContainer *data, const QModelIndex &curIndex)
{
    return pathstr::joinPath(data->m_metadata.workDir, TreeModel::getPath(curIndex));
}

QString DataHelper::branch_path_existing(DataContainer *data, const QModelIndex &subfolder)
{
    if (!TreeModel::isFolderRow(subfolder))
        return QString();

    if (data->m_cacheBranches.contains(subfolder)) {
        const QString fp = data->m_cacheBranches.value(subfolder);
        if (fp.isEmpty() || QFileInfo::exists(fp)) { // in case of renaming
            qDebug() << "DC::branchFilePath >> return cached:" << fp;
            return fp;
        }
    }

    // searching
    QString path = branch_path_composed(data, subfolder);
    if (!QFileInfo::exists(path))
        path = Files::firstDbFile(pathstr::parentFolder(path));

    // caching the result; an empty value means no branches
    data->m_cacheBranches[subfolder] = path;

    return path;
}

// returns the predefined/supposed path to the branch db file, regardless of its existence
QString DataHelper::branch_path_composed(const DataContainer *data, const QModelIndex &subfolder)
{
    const bool isLongExt = pathstr::hasExtension(data->m_metadata.dbFilePath, Lit::sl_db_exts.first());
    QString extension = Lit::sl_db_exts.at(isLongExt ? 0 : 1);
    QString folderPath = itemAbsolutePath(data, subfolder);
    QString fileName = format::composeDbFileName(QStringLiteral(u"checksums"), folderPath, extension);
    QString filePath = pathstr::joinPath(folderPath, fileName);

    return filePath;
}

QString DataHelper::digestFilePath(const DataContainer *data, const QModelIndex &fileIndex)
{
    return paths::digestFilePath(itemAbsolutePath(data, fileIndex), data->m_metadata.algorithm);
}

bool DataHelper::isWorkDirRelative(const DataContainer *data)
{
    return data->m_metadata.workDir == pathstr::parentFolder(data->m_metadata.dbFilePath);
}

bool DataHelper::isFilterApplied(const DataContainer *data)
{
    return data->m_metadata.filter.isEnabled();
}

bool DataHelper::contains(const DataContainer *data, const FileStatuses flags, const QModelIndex &subfolder)
{
    const Numbers &num = TreeModel::isFolderRow(subfolder) ? getNumbers(data, subfolder) : data->m_numbers;

    return num.contains(flags);
}

bool DataHelper::isAllChecked(const DataContainer *data)
{
    return (contains(data, FileStatus::CombChecked)
            && !contains(data, FileStatus::CombNotChecked | FileStatus::CombProcessing));
}

bool DataHelper::isAllMatched(const DataContainer *data, const QModelIndex &subfolder)
{
    const Numbers &nums = TreeModel::isFolderRow(subfolder) ? getNumbers(data, subfolder) : data->m_numbers;

    return isAllMatched(nums);
}

bool DataHelper::isAllMatched(const Numbers &nums)
{
    return (!nums.contains(FileStatus::CombProcessing)
            && nums.numberOf(FileStatus::Matched) == nums.numberOf(FileStatus::CombHasChecksum));
}

bool DataHelper::isDbFileState(const DataContainer *data, DbFileState state)
{
    return (state == data->m_metadata.dbFileState);
}

bool DataHelper::isInCreation(const DataContainer *data)
{
    return (data->m_metadata.dbFileState == MetaData::NoFile);
}

bool DataHelper::isImmutable(const DataContainer *data)
{
    return (data->m_metadata.flags & MetaData::FlagConst);
}

bool DataHelper::hasPossiblyMovedItems(const DataContainer *data)
{
    return contains(data, FileStatus::New) && contains(data, FileStatus::Missing);
}

bool DataHelper::isBackupExists(const DataContainer *data)
{
    return QFileInfo::exists(backupFilePath(data));
}

bool DataHelper::makeBackup(const DataContainer *data, bool forceOverwrite)
{
    if (!QFileInfo::exists(data->m_metadata.dbFilePath))
        return false;

    if (forceOverwrite)
        removeBackupFile(data);

    return QFile::copy(data->m_metadata.dbFilePath,
                       backupFilePath(data));
}

bool DataHelper::restoreBackupFile(const DataContainer *data)
{
    if (isBackupExists(data)) {
        if (QFile::exists(data->m_metadata.dbFilePath)) {
            if (!QFile::remove(data->m_metadata.dbFilePath))
                return false;
        }
        return QFile::rename(backupFilePath(data), data->m_metadata.dbFilePath);
    }
    return false;
}

void DataHelper::removeBackupFile(const DataContainer *data)
{
    if (isBackupExists(data))
        QFile::remove(backupFilePath(data));
}

const Numbers& DataHelper::updateNumbers(DataContainer *data)
{
    data->m_numbers = getNumbers(data);
    return data->m_numbers;
}

Numbers DataHelper::getNumbers(const DataContainer *data, const QModelIndex &rootIndex)
{
    return getNumbers(data->m_model, rootIndex);
}

Numbers DataHelper::getNumbers(const QAbstractItemModel *model, const QModelIndex &rootIndex)
{
    Numbers num;

    TreeModelIterator iter(model, rootIndex);

    while (iter.hasNext()) {
        iter.nextFile();
        num.addFile(iter.status(), iter.size());
    }

    return num;
}
