/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "numbers.h"

Numbers::Numbers() {}

void Numbers::addFile(const FileStatus status, const qint64 size)
{
    _val[status] << size;
}

void Numbers::removeFile(const FileStatus status, const qint64 size)
{
    /*if (_val.contains(status)) {
        _val[status].subtractOne(size);
        if (_val[status]._num == 0)
            _val.remove(status);
    }*/

    if (_val.contains(status)
        && !(_val[status] -= size)) // if the remainder is 0
    {
        _val.remove(status);
    }
}

bool Numbers::moveFile(const FileStatus statusBefore, const FileStatus statusAfter, const qint64 size)
{
    if (!_val.contains(statusBefore) || (statusBefore == statusAfter))
        return false;

    removeFile(statusBefore, size);
    addFile(statusAfter, size);

    return true;
}

bool Numbers::changeStatus(const FileStatus _before, const FileStatus _after)
{
    if (_val.contains(_before)) {
        _val[_after] << _val.take(_before);
        return true;
    }

    return false;
}

bool Numbers::contains(const FileStatuses flag) const
{
    QHash<FileStatus, NumSize>::const_iterator it;

    for (it = _val.constBegin(); it != _val.constEnd(); ++it) {
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

    for (it = _val.constBegin(); it != _val.constEnd(); ++it) {
        if (it.key() & flag)
            _res += it.value();
    }

    return _res;
}
