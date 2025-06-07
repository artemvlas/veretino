/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "tools.h"
#include <QStringBuilder>
#include <QDateTime>
#include <QFileInfo>
#include <cmath>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include "files.h"
#include "pathstr.h"

const QStringList Lit::sl_db_exts = {
    QStringLiteral(u"ver.json"),
    QStringLiteral(u"ver")
};
const QStringList Lit::sl_digest_exts = {
    QStringLiteral(u"md5"),
    QStringLiteral(u"sha1"),
    QStringLiteral(u"sha256"),
    QStringLiteral(u"sha512")
};
const QStringList Lit::sl_digest_Exts = {
    QStringLiteral(u"MD5"),
    QStringLiteral(u"SHA-1"),
    QStringLiteral(u"SHA-256"),
    QStringLiteral(u"SHA-512")
};

const QString Lit::s_webpage = QStringLiteral(u"https://github.com/artemvlas/veretino");
const QString Lit::s_appName = QStringLiteral(APP_NAME);
const QString Lit::s_appNameVersion = QStringLiteral(APP_NAME_VERSION);
const QString Lit::s_app_name = QStringLiteral(u"veretino");
const QString Lit::s_sepStick = QStringLiteral(u" | ");
const QString Lit::s_sepCommaSpace = QStringLiteral(u", ");
const QString Lit::s_sepColonSpace = QStringLiteral(u": ");
const QString Lit::s_dt_format = QStringLiteral(u"yyyy/MM/dd HH:mm");
const QString Lit::s_db_prefix = QStringLiteral(u"checksums");

namespace tools {
int algoStrLen(QCryptographicHash::Algorithm algo)
{
    switch (algo) {
    case QCryptographicHash::Md5:
        return 32;
    case QCryptographicHash::Sha1:
        return 40;
    case QCryptographicHash::Sha256:
        return 64;
    case QCryptographicHash::Sha512:
        return 128;
    default:
        return 0;
    }
}

QCryptographicHash::Algorithm algoByStrLen(int strLen)
{
    switch (strLen) {
    case 32:
        return QCryptographicHash::Md5;
    case 40:
        return QCryptographicHash::Sha1;
    case 64:
        return QCryptographicHash::Sha256;
    case 128:
        return QCryptographicHash::Sha512;
    default:
        return static_cast<QCryptographicHash::Algorithm>(0);
    }
}

QCryptographicHash::Algorithm strToAlgo(const QString &strAlgo)
{
    QList<int> digits;

    for (QChar ch : strAlgo) {
        if (ch.isDigit())
            digits.append(ch.digitValue());
    }

    switch (digitsToNum(digits)) {
        case 1:
            return QCryptographicHash::Sha1;
        case 256:
            return QCryptographicHash::Sha256;
        case 512:
            return QCryptographicHash::Sha512;
        case 5:
            return QCryptographicHash::Md5;
        default:
            return static_cast<QCryptographicHash::Algorithm>(0);
    }
}

int digitsToNum(const QList<int> &digits)
{
    int number = 0;

    for (int dgt : digits) {
        number = (number * 10) + dgt;
    }

    return number;
}

bool canBeChecksum(const QString &str)
{
    static const QSet<int> s_perm_length = {32, 40, 64, 128};

    if (!s_perm_length.contains(str.length())) {
        return false;
    }

    for (int i = 0; i < str.length(); ++i) {
        if (!isHexChar(str.at(i)))
            return false;
    }

    return true;
}

bool canBeChecksum(const QString &str, QCryptographicHash::Algorithm algo)
{
    return (str.size() == algoStrLen(algo)) && canBeChecksum(str);
}

bool isHexChar(const char ch)
{
    return (ch >= '0' && ch <= '9')
           || (ch >= 'A' && ch <= 'F')
           || (ch >= 'a' && ch <= 'f');
}

bool isHexChar(const QChar ch)
{
    return isHexChar(ch.toLatin1());
}

bool isLater(const QString &dt_before, const QString &dt_later)
{
    // format "yyyy/MM/dd HH:mm"
    const int dt_str_len = 16;

    if (dt_before.size() != dt_str_len
        || dt_later.size() != dt_str_len)
    {
        return false;
    }

    for (int i = 0; i < dt_before.size(); ++i) {
        const int ch1v = dt_before.at(i).digitValue();
        const int ch2v = dt_later.at(i).digitValue();

        if (ch1v < ch2v)
            return true;
        if (ch1v > ch2v)
            return false;
    }

    return false;
}

bool isLater(const QString &dt_str, const QDateTime &other)
{
    if (dt_str.isEmpty())
        return false;

    return isLater(dt_str, other.toString(Lit::s_dt_format));
}

bool isFlagCombined(const int flag)
{
    return (flag & (flag - 1));
}

bool isFlagNonCombined(const int flag)
{
    // > 0
    // return flag && !(flag & (flag - 1));

    return !(flag & (flag - 1));
}

QString joinStrings(const QString &str1, const QString &str2, QChar sep)
{
    const bool s1Ends = str1.endsWith(sep);
    const bool s2Starts = str2.startsWith(sep);

    if (s1Ends && s2Starts) {
        QStringView chopped = QStringView(str1).left(str1.size() - 1);
        return chopped % str2;
    }

    if (s1Ends || s2Starts)
        return str1 + str2;

    return str1 % sep % str2;
}

QString joinStrings(const QString &str1, const QString &str2, const QString &sep)
{
    return str1 % sep % str2;
}

QString joinStrings(int num, const QString &str)
{
    return QString::number(num) % ' ' % str;
}

QString joinStrings(const QString &str, int num)
{
    return str % ' ' % QString::number(num);
}

FileStatus failedCalcStatus(const QString &path, bool isChecksumStored)
{
    if (QFileInfo::exists(path))
        return QFileInfo(path).isReadable() ? FileStatus::ReadError : FileStatus::UnPermitted;

    return isChecksumStored ? FileStatus::Missing : FileStatus::Removed;
}
} // namespace tools

namespace paths {
QString digestFilePath(const QString &file, QCryptographicHash::Algorithm algo)
{
    const QString ext = format::algoToStr(algo, false);
    return tools::joinStrings(file, ext, u'.');
}

QString digestFilePath(const QString &file, const int sum_len)
{
    return digestFilePath(file, tools::algoByStrLen(sum_len));
}

bool isDbFile(const QString &filePath)
{
    return pathstr::hasExtension(filePath, Lit::sl_db_exts);
}

bool isDigestFile(const QString &filePath)
{
    return pathstr::hasExtension(filePath, Lit::sl_digest_exts);
}

void browsePath(const QString &path)
{
    if (QFile::exists(path)) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}
} // namespace paths

namespace format {
QString currentDateTime()
{
    return QDateTime::currentDateTime().toString(Lit::s_dt_format);
}

QString numString(qint64 num)
{
    QString numstr = QString::number(num);

    for (int i = numstr.size() - 3; i > 0; i -= 3) {
        numstr.insert(i, ',');
    }

    return numstr;
}

QString millisecToReadable(qint64 milliseconds, bool approx)
{
    int seconds = milliseconds / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;
    int hours = minutes / 60;
    minutes = minutes % 60;

    if (hours > 0) {
        return approx ? QString("%1 h %2 min").arg(hours).arg(minutes)
                      : QString("%1 h %2 min %3 sec").arg(hours).arg(minutes).arg(seconds);
    }

    if (approx && minutes > 0 && seconds > 15) {
        return tools::joinStrings(++minutes, QStringLiteral(u"min"));
    }

    if (minutes > 0) {
        return approx ? tools::joinStrings(minutes, QStringLiteral(u"min"))
                      : QString("%1 min %2 sec").arg(minutes).arg(seconds);
    }

    return (approx && seconds < 5) ? QStringLiteral(u"few sec")
                                   : tools::joinStrings(seconds, QStringLiteral(u"sec"));
}

QString dataSizeReadable(const qint64 sizeBytes)
{
    if (sizeBytes <= 1000)
        return tools::joinStrings((int)sizeBytes, QStringLiteral(u"bytes"));

    long double converted = sizeBytes;
    int it = 0; // number of divisions

    do {
        converted /= 1024;
        ++it;
    } while (converted > 1000);

    QChar ch;
    switch (it) {
    case 1:
        ch = u'K';
        break;
    case 2:
        ch = u'M';
        break;
    case 3:
        ch = u'G';
        break;
    case 4:
        ch = u'T';
        break;
    default:
        ch = u'?';
        break;
    }

    const float x = std::round(converted * 100) / 100;
    return QString::number(x, 'f', 2) % ch % QStringLiteral(u"iB");
}

QString dataSizeReadableExt(const qint64 sizeBytes)
{
    return QString("%1 (%2 bytes)").arg(dataSizeReadable(sizeBytes), numString(sizeBytes));
}

QString shortenString(const QString &string, int length, bool cutEnd)
{
    if (string.length() <= length)
        return string;

    QString dots = QStringLiteral(u"...");

    return cutEnd ? string.left(length).append(dots)
                  : string.right(length).prepend(dots);
}

QString simplifiedChars(QString str)
{
    /*
     * str = str.simplified();
     * static const QRegularExpression re("[ /\\\\:;%*?|<>&#~^]");
     * str.replace(re, "_");
     */

    if (str.isEmpty())
        return str;

    static const QString forbSymb(" :/\\%*?|<>&#^");

    for (int i = 0; i < str.size(); ++i) {
        if (forbSymb.contains(str.at(i))) {
            str[i] = u'_';
        }
    }

    while (str.contains(QStringLiteral(u"__")))
        str.replace(QStringLiteral(u"__"), QStringLiteral(u"_"));

    return str;
}

QString inParentheses(const int number)
{
    return inParentheses(QString::number(number));
}

QString inParentheses(const QString &str)
{
    return '(' % str % ')';
}

QString addStrInParentheses(const QString &str1, const QString &str2)
{
    return str1 % QStringLiteral(u" (") % str2 % ')';
}

QString composeDbFileName(const QString &prefix, const QString &folder, const QString &extension)
{
    if (folder.isEmpty())
        return tools::joinStrings(prefix, extension, u'.');

    const QString folderStr = simplifiedChars(pathstr::basicName(folder));
    const QString dbFileName = tools::joinStrings(prefix, folderStr, u'_');

    return tools::joinStrings(dbFileName, extension, u'.');
}

QString algoToStr(QCryptographicHash::Algorithm algo, bool capitalLetters)
{
    const QStringList &lst = capitalLetters ? Lit::sl_digest_Exts : Lit::sl_digest_exts;

    switch (algo) {
    case QCryptographicHash::Md5:
        return lst.at(0);
    case QCryptographicHash::Sha1:
        return lst.at(1);
    case QCryptographicHash::Sha256:
        return lst.at(2);
    case QCryptographicHash::Sha512:
        return lst.at(3);
    default:
        return "Unknown";
    }
}

QString algoToStr(int sumStrLength, bool capitalLetters)
{
    return algoToStr(tools::algoByStrLen(sumStrLength), capitalLetters);
}

QString filesNumber(int number)
{
    if (number == 0)
        return QStringLiteral(u"no files");

    const QString files = (number == 1) ? QStringLiteral(u"file") : QStringLiteral(u"files");

    return tools::joinStrings(number, files);
}

QString filesNumSize(int number, qint64 filesSize)
{
    if (number == 0)
        return filesNumber(number);

    return addStrInParentheses(filesNumber(number), dataSizeReadable(filesSize));
}

QString filesNumSize(const Numbers &num, FileStatus status)
{
    return filesNumSize(num.values(status));
}

QString filesNumSize(const NumSize &nums)
{
    return filesNumSize(nums._num, nums._size);
}

QString fileNameAndSize(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    return addStrInParentheses(fileInfo.fileName(), dataSizeReadable(fileInfo.size()));
}

QString fileNameAndSize(const QString &file, const qint64 size)
{
    return addStrInParentheses(pathstr::basicName(file), dataSizeReadable(size));
}

QString fileItemStatus(FileStatus status)
{
    switch (status) {
    case FileStatus::Queued: return QStringLiteral(u"queued");
    case FileStatus::Calculating: return QStringLiteral(u"calculating...");
    case FileStatus::Verifying: return QStringLiteral(u"verifying...");
    case FileStatus::NotChecked: return QStringLiteral(u"ready");
    case FileStatus::NotCheckedMod: return QStringLiteral(u"ready (modif.)");
    case FileStatus::Matched: return QStringLiteral(u"match");
    case FileStatus::Mismatched: return QStringLiteral(u"not match");
    case FileStatus::New: return QStringLiteral(u"new");
    case FileStatus::Missing: return QStringLiteral(u"missing");
    case FileStatus::Added: return QStringLiteral(u"added");
    case FileStatus::Removed: return QStringLiteral(u"removed");
    case FileStatus::Updated: return QStringLiteral(u"updated");
    case FileStatus::Imported: return QStringLiteral(u"imported");
    case FileStatus::Moved: return QStringLiteral(u"moved");
    case FileStatus::MovedOut: return QStringLiteral(u"moved out");
    case FileStatus::UnPermitted: return QStringLiteral(u"no permissions");
    case FileStatus::ReadError: return QStringLiteral(u"read error");
    default: return "unknown";
    }
}

QString coloredText(bool ignore)
{
    QString color = ignore ? QStringLiteral(u"red") : QStringLiteral(u"green");
    return QStringLiteral(u"color : ") + color;
}

QString coloredText(const QString &className, bool ignore)
{
    // "%1 { %2 }"
    return className % QStringLiteral(u" { ") % coloredText(ignore) % QStringLiteral(u" }");
}

} // namespace format
