/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef NUMBERS_H
#define NUMBERS_H

#include "files.h"

class Numbers
{
public:
    Numbers();

    void addFile(const FileStatus status, const qint64 size);
    void removeFile(const FileStatus status, const qint64 size);
    bool moveFile(const FileStatus statusBefore,
                  const FileStatus statusAfter,
                  const qint64 size = 0);

    bool contains(const FileStatuses flag) const;
    int numberOf(const FileStatuses flag) const;
    qint64 totalSize(const FileStatuses flag) const;
    NumSize values(const FileStatuses flag) const;

    bool changeStatus(const FileStatus before,                         // assigns new status to numbers
                      const FileStatus after);

    bool changeStatus(const NumSize &nums,                             // moves the specified value to a new status
                      const FileStatus before,
                      const FileStatus after);

    const QList<FileStatus> statuses() const;                          // returns a list of available statuses
    void clear();                                                      // clears the _val

private:
    // { FileStatus : number of corresponding files, total size }
    QHash<FileStatus, NumSize> _val;
}; // class Numbers

#endif // NUMBERS_H
