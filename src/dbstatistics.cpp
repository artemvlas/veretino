#include "dbstatistics.h"
#include "treemodeliterator.h"
#include "pathstr.h"
#include <QFileInfo>

DbStatistics::DbStatistics() {}

DbStatistics::DbStatistics(const DataContainer *data)
    : m_data(data) {}

void DbStatistics::setData(const DataContainer *data)
{
    m_data = data;
    m_numbers.clear();
}

void DbStatistics::clear()
{
    setData(nullptr);
}

const DataContainer* DbStatistics::data() const
{
    return m_data;
}

const MetaData& DbStatistics::metadata() const
{
    return m_data->m_metadata;
}

const Numbers& DbStatistics::updateNumbers()
{
    m_numbers = getNumbers();
    return m_numbers;
}

Numbers DbStatistics::getNumbers(const QModelIndex &rootIndex) const
{
    if (!m_data)
        return {};

    Numbers num;

    TreeModelIterator iter(m_data->m_model, rootIndex);

    while (iter.hasNext()) {
        iter.nextFile();
        num.addFile(iter.status(), iter.size());
    }

    return num;
}

bool DbStatistics::contains(const FileStatuses flags, const QModelIndex &subfolder) const
{
    const Numbers &num = TreeModel::isFolderRow(subfolder) ? getNumbers(subfolder) : m_numbers;

    return num.contains(flags);
}

bool DbStatistics::checkCondition(Condition condition) const
{
    if (!m_data)
        return false;

    switch (condition) {
    case NoDbFile:
    //case InCreation:
        return (m_data->m_metadata.dbFileState == MetaData::NoFile);
    case DbFileExists:
        return QFileInfo::exists(m_data->m_metadata.dbFilePath);
    case NotSaved:
        return (m_data->m_metadata.dbFileState == DbFileState::NotSaved);
    case Immutable:
        return (m_data->m_metadata.flags & MetaData::FlagConst);
    case WorkDirRelative:
        return (m_data->m_metadata.workDir == pathstr::parentFolder(m_data->m_metadata.dbFilePath));
    case FilterApplied:
        return m_data->m_metadata.filter.isEnabled();
    case AllChecked:
        return (contains(FileStatus::CombChecked)
                && !contains(FileStatus::CombNotChecked | FileStatus::CombProcessing));
    case AllMatched:
        return isAllMatched(m_numbers);
    case HasPossiblyMovedItems:
        return contains(FileStatus::New) && contains(FileStatus::Missing);
    default:
        break;
    }

    return false;
}

bool DbStatistics::isAllMatched(const QModelIndex &subfolder) const
{
    const Numbers &nums = TreeModel::isFolderRow(subfolder) ? getNumbers(subfolder) : m_numbers;

    return isAllMatched(nums);
}

bool DbStatistics::isAllMatched(const Numbers &nums)
{
    return (!nums.contains(FileStatus::CombProcessing)
            && nums.numberOf(FileStatus::Matched) == nums.numberOf(FileStatus::CombHasChecksum));
}

bool DbStatistics::isDbFileState(DbFileState state) const
{
    return m_data && (state == m_data->m_metadata.dbFileState);
}
