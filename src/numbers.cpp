/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "numbers.h"

Numbers::Numbers() {}

void Numbers::addFile(const FileStatus status, const qint64 size)
{
    val_[status] << size;
}

void Numbers::removeFile(const FileStatus status, const qint64 size)
{
    /*if (val_.contains(status)) {
        val_[status].subtractOne(size);
        if (val_[status]._num == 0)
            val_.remove(status);
    }*/

    if (val_.contains(status)
        && !(val_[status] -= size)) // 0
    {
        val_.remove(status);
    }
}

bool Numbers::moveFile(const FileStatus statusBefore, const FileStatus statusAfter, const qint64 size)
{
    if (!val_.contains(statusBefore) || (statusBefore == statusAfter))
        return false;

    removeFile(statusBefore, size);
    addFile(statusAfter, size);

    return true;
}

bool Numbers::contains(const FileStatuses flag) const
{
    QHash<FileStatus, NumSize>::const_iterator it;

    for (it = val_.constBegin(); it != val_.constEnd(); ++it) {
        if ((flag & it.key()) && it.value())
            return true;
    }

    return false;
}

int Numbers::numberOf(const FileStatuses flag) const
{
    return values(flag)._num;
}

qint64 Numbers::totalSize(const FileStatuses flag) const
{
    return values(flag)._size;
}

NumSize Numbers::values(const FileStatuses flag) const
{
    NumSize _res;
    QHash<FileStatus, NumSize>::const_iterator it;

    for (it = val_.constBegin(); it != val_.constEnd(); ++it) {
        if (it.key() & flag)
            _res += it.value();
    }

    return _res;
}
