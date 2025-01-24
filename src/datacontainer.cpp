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
    : QObject(parent)
{
    createModels();
}

DataContainer::DataContainer(const MetaData &meta, QObject *parent)
    : QObject(parent), m_metadata(meta)
{
    createModels();
}

DataContainer::DataContainer(const MetaData &meta, TreeModel *data, QObject *parent)
    : QObject(parent)
{
    setData(meta, data);
}

void DataContainer::setData()
{
    clearData();
    createModels();
}

void DataContainer::setData(const MetaData &meta, TreeModel *data)
{
    clearData();
    p_model = data;
    p_model->setParent(this);
    p_proxy = new ProxyModel(p_model, this);

    m_metadata = meta;
}

bool DataContainer::hasData() const
{
    return p_model && !p_model->isEmpty();
}

void DataContainer::createModels()
{
    p_model = new TreeModel(this);
    p_proxy = new ProxyModel(p_model, this);
}

void DataContainer::clearData()
{
    if (p_proxy) {
        delete p_proxy;
        p_proxy = nullptr;
    }
    if (p_model) {
        delete p_model;
        p_model = nullptr;
    }

    m_numbers.clear();
}

QString DataContainer::databaseFileName() const
{
    return pathstr::basicName(m_metadata.dbFilePath);
}

QString DataContainer::backupFilePath() const
{
    using namespace pathstr;
    return joinPath(parentFolder(m_metadata.dbFilePath),
                    QStringLiteral(u".tmp-backup_") + basicName(m_metadata.dbFilePath));
}

// returns the absolute path to the db item (file or subfolder)
// (working folder path in filesystem + relative path in the database)
QString DataContainer::itemAbsolutePath(const QModelIndex &curIndex) const
{
    return pathstr::joinPath(m_metadata.workDir, TreeModel::getPath(curIndex));
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
    const bool _isLongExt = pathstr::hasExtension(m_metadata.dbFilePath, Lit::sl_db_exts.first());
    QString extension = Lit::sl_db_exts.at(_isLongExt ? 0 : 1);
    QString folderPath = itemAbsolutePath(subfolder);
    QString fileName = format::composeDbFileName(QStringLiteral(u"checksums"), folderPath, extension);
    QString filePath = pathstr::joinPath(folderPath, fileName);

    return filePath;
}

QString DataContainer::digestFilePath(const QModelIndex &fileIndex) const
{
    return paths::digestFilePath(itemAbsolutePath(fileIndex), m_metadata.algorithm);
}

bool DataContainer::isWorkDirRelative() const
{
    return (pathstr::parentFolder(m_metadata.dbFilePath) == m_metadata.workDir);
}

bool DataContainer::isFilterApplied() const
{
    return m_metadata.filter.isEnabled();
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
    return (state == m_metadata.dbFileState);
}

bool DataContainer::isInCreation() const
{
    return (m_metadata.dbFileState == MetaData::NoFile);
}

bool DataContainer::isImmutable() const
{
    return (m_metadata.flags & MetaData::FlagConst);
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
    if (!QFileInfo::exists(m_metadata.dbFilePath))
        return false;

    if (forceOverwrite)
        removeBackupFile();

    return QFile::copy(m_metadata.dbFilePath,
                       backupFilePath());
}

bool DataContainer::restoreBackupFile() const
{
    if (isBackupExists()) {
        if (QFile::exists(m_metadata.dbFilePath)) {
            if (!QFile::remove(m_metadata.dbFilePath))
                return false;
        }
        return QFile::rename(backupFilePath(), m_metadata.dbFilePath);
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

DataContainer::~DataContainer()
{
    removeBackupFile();
    qDebug() << Q_FUNC_INFO << databaseFileName();
}
