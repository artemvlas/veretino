/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DBSTATISTICS_H
#define DBSTATISTICS_H

#include "datacontainer.h"
#include "numbers.h"

class DbStatistics
{
public:
    DbStatistics();
    explicit DbStatistics(const DataContainer *data);

    void setData(const DataContainer *data);
    void clear();

    const DataContainer* data() const;
    const MetaData& metadata() const;

    Numbers getNumbers(const QModelIndex &rootIndex = QModelIndex()) const;
    const Numbers& updateNumbers();

    bool contains(const FileStatuses flags,
                  const QModelIndex &subfolder = QModelIndex()) const;

    bool isAllMatched(const QModelIndex &subfolder = QModelIndex()) const;
    static bool isAllMatched(const Numbers &nums);

    bool isDbFileState(DbFileState state) const;

    enum Condition {
        // DB
        NoDbFile = 1 << 0,
        InCreation = NoDbFile,
        DbFileExists = 1 << 1,
        NotSaved = 1 << 2,
        Immutable = 1 << 3, // has FlagConst
        WorkDirRelative = 1 << 4,
        FilterApplied = 1 << 5,

        // File Items
        AllChecked = 1 << 6,
        AllMatched = 1 << 7,
        HasPossiblyMovedItems = 1 << 8, // has New and Missing items
    };

    bool checkCondition(Condition condition) const;

    Numbers m_numbers;

private:
    const DataContainer *m_data = nullptr;
};

#endif // DBSTATISTICS_H
