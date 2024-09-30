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
#include "numbers.h"

struct Lit { // Literals
static const QStringList sl_db_exts;
static const QStringList sl_digest_exts;
static const QString s_webpage;
static const QString s_appName;
static const QString s_appNameVersion;
static const QString s_app_name;
static const QString s_sepStick;
static const QString s_dt_format;
}; // struct Lit

namespace tools {
int algoStrLen(QCryptographicHash::Algorithm algo); // returns the length of checksum string depending on the sha-type: sha(1) = 40, sha(256) = 64, sha(512) = 128
QCryptographicHash::Algorithm algorithmByStrLen(int strLen); // ^vice versa
QCryptographicHash::Algorithm strToAlgo(const QString &strAlgo);
int digitsToNum(const QList<int> &digits); // {0,1,2,3} --> 123

bool isDatabaseFile(const QString &filePath);
bool isSummaryFile(const QString &filePath);
bool canBeChecksum(const QString &str);
bool isLater(const QString &dt_before, const QString &dt_later); // true ("2024/09/24 18:35", "2024/09/25 11:40")
bool isLater(const QString &dt_str, const QDateTime &other);

QString joinStrings(const QString &str1, const QString &str2, QChar sep); // checks for the absence of sep duplication
QString joinStrings(const QString &str1, const QString &str2, const QString &sep); // no such check
QString joinStrings(int num, const QString &str); // --> "X str"
QString joinStrings(const QString &str, int num); // --> "str X"

FileStatus failedCalcStatus(const QString &path, bool isChecksumStored = false);
} // namespace tools

namespace paths {
static const QChar _sep = u'/';
static const QChar _dot = u'.';
QString parentFolder(const QString &path); // returns the parent folder of the 'path'
QString basicName(const QString &path); // returns file or folder name: "/home/user/folder/fname" --> "fname"
QString relativePath(const QString &rootFolder, const QString &fullPath);
QString shortenPath(const QString &path); // if not a root (or child of root) path, returns "../path"
QString joinPath(const QString &absolutePath, const QString &addPath); // returns '/absolutePath/addPath'
QString composeFilePath(const QString &parentFolder, const QString &fileName, const QString &ext);
bool isRoot(const QString &path); // true: "/" or "X:'/'"; else false
bool hasExtension(const QString &file, const QString &ext); // true if the "file" (name or path) have the "ext" suffix
bool hasExtension(const QString &file, const QStringList &extensions); // true if the file have any extension from the list
void browsePath(const QString &path);
} // namespace paths

namespace format {
QString currentDateTime();

QString numString(qint64 num); // Returns a string of numbers separated by commas: 1,234,567,890
QString millisecToReadable(qint64 milliseconds, bool approx = false); // converts milliseconds to readable time like "1 min 23 sec"
QString dataSizeReadable(const qint64 sizeBytes); // converts size in bytes to human readable form like "129.17 GiB"
QString dataSizeReadableExt(const qint64 sizeBytes); // returning style example: "6.08 GiB (6,532,974,324 bytes)"
QString shortenString(const QString &string, int length = 64, bool cutEnd = true);
QString simplifiedChars(QString str);
QString inParentheses(const int number);
QString inParentheses(const QString &str); // returns "(str)"
QString addStrInParentheses(const QString &str1, const QString &str2); // returns "str1 (str2)"
QString composeDbFileName(const QString &prefix, const QString &folder, const QString &extension);
QString algoToStr(QCryptographicHash::Algorithm algo, bool capitalLetters = true);
QString algoToStr(int sumStrLength, bool capitalLetters = true);

QString fileNameAndSize(const QString &filePath); // returns "filename (readable size)" for file
QString filesNumber(int number);
QString filesNumSize(int number, qint64 filesSize); // returns "number file's' (readable size)"
QString filesNumSize(const Numbers &num, FileStatus status);
QString fileItemStatus(FileStatus status);

QString coloredText(bool ignore); // 'ignore' (true = red, false = green)
QString coloredText(const QString &className, bool ignore); // 'className': "QLineEdit", "QTreeView", "QLabel", etc...
} // namespace format

#endif // TOOLS_H
