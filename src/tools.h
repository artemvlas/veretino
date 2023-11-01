// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef TOOLS_H
#define TOOLS_H

#include <QString>
#include <QCryptographicHash>
#include <QAbstractItemModel>
#include "files.h"

struct Settings {
    QCryptographicHash::Algorithm algorithm = QCryptographicHash::Sha256;
    FilterRule filter;
    QString dbPrefix = "checksums";
};

namespace tools {
int algoStrLen(QCryptographicHash::Algorithm algo); // returns the length of checksum string depending on the sha-type: sha(1) = 40, sha(256) = 64, sha(512) = 128
QCryptographicHash::Algorithm algorithmByStrLen(int strLen); // ^vice versa
QCryptographicHash::Algorithm strToAlgo(const QString &strAlgo);

bool isDatabaseFile(const QString &filePath);
bool isSummaryFile(const QString &filePath);
bool canBeChecksum(const QString &text);
} // namespace tools

namespace paths {
QString parentFolder(const QString &path); // returns the parent folder of the 'path'
QString basicName(const QString &path); // returns file or folder name: "/home/user/folder/fname" --> "fname"
QString joinPath(const QString &absolutePath, const QString &addPath); // returns '/absolutePath/addPath'
QString backupFilePath(const QString &filePath);

bool isFileAllowed(const QString &filePath, const FilterRule &filter); // whether the file extension matches the filter rules
} // namespace paths

namespace ModelKit {
enum ItemDataRoles {RawDataRole = 1000};
enum Columns {PathColumn, SizeColumn, StatusColumn, ChecksumColumn};

QString getPath(const QModelIndex &curIndex); // build path by current index data
QModelIndex getIndex(const QString &path, const QAbstractItemModel *model); // find index of specified 'path'
QModelIndex getRowItemIndex(const QModelIndex &curIndex, ModelKit::Columns column); // get the index of an item of the same row (curIndex row) and a specified column

bool isFileRow(const QModelIndex &curIndex); // whether the row of curIndex corresponds to a file(true) or folder(false)
} // namespace ModelKit

namespace format {
QString currentDateTime();

QString numString(qint64 num); // Returns a string of numbers separated by commas: 1,234,567,890
QString dataSizeReadable(qint64 sizeBytes); // converts size in bytes to human readable form like "129.17 GiB"
QString dataSizeReadableExt(qint64 sizeBytes); // returning style example: "6.08 GiB (6,532,974,324 bytes)"
QString shortenString(const QString &string, int length = 64);
QString algoToStr(QCryptographicHash::Algorithm algo);

QString fileNameAndSize(const QString &filePath); // returns "filename (readable size)" for file
QString filesNumberAndSize(int filesNumber, qint64 filesSize); // returns "number file's' (readable size)"

QString fileItemStatus(int status);
} // namespace format

namespace Mode {
enum Modes {
    NoMode,
    Folder,
    File,
    DbFile,
    SumFile,
    Model,
    ModelNewLost,
    UpdateMismatch,
    Processing,
    EndProcess
};
} // namespace Mode

#endif // TOOLS_H
