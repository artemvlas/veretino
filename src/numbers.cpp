/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "numbers.h"

Numbers::Numbers() {}

void Numbers::addFile(const FileStatus status, const qint64 size)
{
    amounts_[status]++;
    sizes_[status] += size;
}

bool Numbers::contains(const FileStatuses flags) const
{
    return numberOf(flags) > 0;
}

int Numbers::numberOf(const FileStatuses flag) const
{
    int result = 0;
    QHash<FileStatus, int>::const_iterator it;

    for (it = amounts_.constBegin(); it != amounts_.constEnd(); ++it) {
        if (it.key() & flag)
            result += it.value();
    }

    return result;
}

qint64 Numbers::totalSize(const FileStatuses flag) const
{
    qint64 result = 0;
    QHash<FileStatus, qint64>::const_iterator it;

    for (it = sizes_.constBegin(); it != sizes_.constEnd(); ++it) {
        if (it.key() & flag) {
            result += it.value();
        }
    }

    return result;
}
