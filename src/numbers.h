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
    bool moveFile(const FileStatus statusBefore, const FileStatus statusAfter, const qint64 size);
    bool contains(const FileStatuses flags) const;
    int numberOf(const FileStatuses flag) const;
    qint64 totalSize(const FileStatuses flag) const;
    NumSize values(const FileStatuses flag) const;

private:
    QHash<FileStatus, NumSize> val_;
    //QHash<FileStatus, int> amounts_; // { FileStatus : number of corresponding files }
    //QHash<FileStatus, qint64> sizes_; // { FileStatus : total size }
}; // class Numbers

#endif // NUMBERS_H
