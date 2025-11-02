/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef TOOLS_H
#define TOOLS_H

/*** Error codes ***/
#define ERR_OK 0             /* No errors */
#define ERR_ERROR -1         /* Error of some kind */
#define ERR_READ -2          /* Error while reading file */
#define ERR_WRITE -3         /* Error while writing to file */
#define ERR_CANCELED -4      /* Process canceled */
#define ERR_NOPERM -5        /* No read or write permissions */
#define ERR_NOTEXIST -6      /* Non-existent file path */
#define ERR_NODATA -7        /* No source or result data */

#include <QString>
#include <QCryptographicHash>
#include <QAbstractItemModel>
#include <QPalette>
#include <QMetaEnum>
#include <QFile>
#include "filevalues.h"
#include "numbers.h"

struct Lit { // Literals
static const QStringList sl_db_exts;
static const QStringList sl_digest_exts;
static const QStringList sl_digest_Exts;
static const QString s_webpage;
static const QString s_appName;
static const QString s_appNameVersion;
static const QString s_app_name;
static const QString s_sepStick;
static const QString s_sepCommaSpace;
static const QString s_sepColonSpace;
static const QString s_dt_format;
static const QString s_db_prefix;
}; // struct Lit

class Exception : public std::runtime_error {
public:
    int errorCode;
    Exception(int code, const std::string& msg = "")
        : std::runtime_error(msg), errorCode(code) {}
}; // class Exception

namespace tools {
// returns the checksum str length: sha(1) = 40, sha(256) = 64, sha(512) = 128
int algoStrLen(QCryptographicHash::Algorithm algo);

// ^vice versa
QCryptographicHash::Algorithm algoByStrLen(int strLen);
QCryptographicHash::Algorithm strToAlgo(const QString &strAlgo);

// {0,1,2,3} --> 123
int digitsToNum(const QList<int> &digits);

bool canBeChecksum(const QString &str);
bool canBeChecksum(const QString &str, QCryptographicHash::Algorithm algo);

// whether the char is a digit or a letter from 'Aa' to 'Ff'
bool isHexChar(const char ch);
bool isHexChar(const QChar ch);

// true ("2024/09/24 18:35", "2024/09/25 11:40")
bool isFlagCombined(const int flag);
bool isFlagNonCombined(const int flag);
bool isLater(const QString &dt_before, const QString &dt_later);
bool isLater(const QString &dt_str, const QDateTime &other);

// checks for the absence of sep duplication
QString joinStrings(const QString &str1, const QString &str2, QChar sep);

// no such check
QString joinStrings(const QString &str1, const QString &str2, const QString &sep);

// --> "X str"
QString joinStrings(int num, const QString &str);

// --> "str X"
QString joinStrings(const QString &str, int num);

QString extractDigestFromFile(const QString &digest_file);

FileStatus failedCalcStatus(const QString &path, bool isChecksumStored = false);

// try to open the 'file'; if an error occurs, throws exceptions
void openFile(QFile &file, QFile::OpenMode mode = QFile::ReadOnly);

template<typename QEnum>
QString enumToString(const QEnum value)
{
    return QMetaEnum::fromType<QEnum>().valueToKey(value);
}
} // namespace tools

namespace paths {
// ../folder/file.txt --> ../folder/file.txt.shaX
QString digestFilePath(const QString &file, QCryptographicHash::Algorithm algo);
QString digestFilePath(const QString &file, const int sum_len);
bool isDbFile(const QString &filePath);
bool isDigestFile(const QString &filePath);
void browsePath(const QString &path);
} // namespace paths

namespace format {
QString currentDateTime();

// Returns a string of numbers separated by commas: 1,234,567,890
QString numString(qint64 num);

// converts milliseconds to readable time like "1 min 23 sec"
QString msecsToReadable(qint64 milliseconds, bool approx = false);

// converts size in bytes to human readable form like "129.17 GiB"
QString dataSizeReadable(const qint64 sizeBytes);

// returning style example: "6.08 GiB (6,532,974,324 bytes)"
QString dataSizeReadableExt(const qint64 sizeBytes);

// returns the readable string of the process speed
QString processSpeed(qint64 msecs, qint64 size);

QString shortenString(const QString &string, int length = 64, bool cutEnd = true);
QString simplifiedChars(QString str);
QString inParentheses(const int number);

// returns "(str)"
QString inParentheses(const QString &str);

// returns "str1 (str2)"
QString addStrInParentheses(const QString &str1, const QString &str2);
QString composeDbFileName(const QString &prefix,
                          const QString &folder, const QString &extension);
QString algoToStr(QCryptographicHash::Algorithm algo, bool capitalLetters = true);
QString algoToStr(int sumStrLength, bool capitalLetters = true);

// returns "filename (readable size)" for file
QString fileNameAndSize(const QString &filePath);
QString fileNameAndSize(const QString &file, const qint64 size);
QString filesNumber(int number);

// returns "number file's' (readable size)"
QString filesNumSize(int number, qint64 filesSize);
QString filesNumSize(const Numbers &num, FileStatus status);
QString filesNumSize(const NumSize &nums);
QString fileItemStatus(FileStatus status);

// 'ignore' (true = red, false = green)
QString coloredText(bool ignore);

// 'className': "QLineEdit", "QTreeView", "QLabel", etc...
QString coloredText(const QString &className, bool ignore);
} // namespace format

#endif // TOOLS_H
