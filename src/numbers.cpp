/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "numbers.h"

Numbers::Numbers() {}

void Numbers::addFile(const FileStatus status, const qint64 size)
{
    /*amounts_[status]++;

    if (size > 0)
        sizes_[status] += size;*/

    val_[status] << size;
}

void Numbers::removeFile(const FileStatus status, const qint64 size)
{
    /*if (amounts_.contains(status)) {
        amounts_[status]--;
        if (amounts_[status] == 0) {
            amounts_.remove(status);
            sizes_.remove(status);
        }
    }

    if (size > 0 && sizes_.contains(status)) {
        sizes_[status] -= size;
    }*/
    if (val_.contains(status)) {
        val_[status].subtractOne(size);
        if (val_[status].num == 0)
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

bool Numbers::contains(const FileStatuses flags) const
{
    return numberOf(flags) > 0;
}

int Numbers::numberOf(const FileStatuses flag) const
{
    /*int result = 0;
    QHash<FileStatus, int>::const_iterator it;

    for (it = amounts_.constBegin(); it != amounts_.constEnd(); ++it) {
        if (it.key() & flag)
            result += it.value();
    }

    return result;*/
    return values(flag).num;
}

qint64 Numbers::totalSize(const FileStatuses flag) const
{
    /*qint64 result = 0;
    QHash<FileStatus, qint64>::const_iterator it;

    for (it = sizes_.constBegin(); it != sizes_.constEnd(); ++it) {
        if (it.key() & flag) {
            result += it.value();
        }
    }

    return result;*/
    return values(flag).size;
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
