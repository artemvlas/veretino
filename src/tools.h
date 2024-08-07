/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef TOOLS_H
#define TOOLS_H

#include <QString>
#include <QCryptographicHash>
#include <QAbstractItemModel>
#include <QPalette>
#include "files.h"

namespace tools {
int algoStrLen(QCryptographicHash::Algorithm algo); // returns the length of checksum string depending on the sha-type: sha(1) = 40, sha(256) = 64, sha(512) = 128
QCryptographicHash::Algorithm algorithmByStrLen(int strLen); // ^vice versa
QCryptographicHash::Algorithm strToAlgo(const QString &strAlgo);

bool isDatabaseFile(const QString &filePath);
bool isSummaryFile(const QString &filePath);
bool canBeChecksum(const QString &str);

QStringList strToList(const QString &str); // finds a common separator and splits the string
} // namespace tools

namespace paths {
QString parentFolder(const QString &path); // returns the parent folder of the 'path'
QString basicName(const QString &path); // returns file or folder name: "/home/user/folder/fname" --> "fname"
QString shortenPath(const QString &path); // if not a root (or child of root) path, returns "../path"
QString joinPath(const QString &absolutePath, const QString &addPath); // returns '/absolutePath/addPath'
void browsePath(const QString &path);
bool isRoot(const QString &path); // true: "/" or "C:/" or "C:"; else false
} // namespace paths

namespace format {
QString currentDateTime();

QString numString(qint64 num); // Returns a string of numbers separated by commas: 1,234,567,890
QString millisecToReadable(qint64 milliseconds, bool approx = false); // converts milliseconds to readable time like "1 min 23 sec"
QString dataSizeReadable(qint64 sizeBytes); // converts size in bytes to human readable form like "129.17 GiB"
QString dataSizeReadableExt(qint64 sizeBytes); // returning style example: "6.08 GiB (6,532,974,324 bytes)"
QString shortenString(const QString &string, int length = 64, bool cutEnd = true);
QString simplifiedChars(QString str);
QString joinStrings(const QString &str1, const QString &str2, const QString joint = "_");
QString composeDbFileName(const QString &prefix, const QString &folderName, const QString &extension);
QString algoToStr(QCryptographicHash::Algorithm algo, bool capitalLetters = true);
QString algoToStr(int sumStrLength, bool capitalLetters = true);

QString fileNameAndSize(const QString &filePath); // returns "filename (readable size)" for file
QString filesNumberAndSize(int filesNumber, qint64 filesSize); // returns "number file's' (readable size)"
QString fileItemStatus(FileStatus status);

QString coloredText(bool ignore); // 'ignore' (true = red, false = green)
QString coloredText(const QString &className, bool ignore); // 'className': "QLineEdit", "QTreeView", "QLabel", etc...
} // namespace format

#endif // TOOLS_H
