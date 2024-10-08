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

const QStringList Lit::sl_db_exts = {
    QStringLiteral(u"ver.json"),
    QStringLiteral(u"ver")
};
const QStringList Lit::sl_digest_exts = {
    QStringLiteral(u"sha1"),
    QStringLiteral(u"sha256"),
    QStringLiteral(u"sha512")
};
const QString Lit::s_webpage = QStringLiteral(u"https://github.com/artemvlas/veretino");
const QString Lit::s_appName = QStringLiteral(APP_NAME);
const QString Lit::s_appNameVersion = QStringLiteral(APP_NAME_VERSION);
const QString Lit::s_app_name = QStringLiteral(u"veretino");
const QString Lit::s_sepStick = QStringLiteral(u" | ");
const QString Lit::s_dt_format = QStringLiteral(u"yyyy/MM/dd HH:mm");
const QString Lit::s_db_prefix = QStringLiteral(u"checksums");

namespace tools {
int algoStrLen(QCryptographicHash::Algorithm algo)
{
    switch (algo) {
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
        case 40:
            return QCryptographicHash::Sha1;
        case 64:
            return QCryptographicHash::Sha256;
        case 128:
            return QCryptographicHash::Sha512;
        default:
            return QCryptographicHash::Sha256;
    }
}

QCryptographicHash::Algorithm strToAlgo(const QString &strAlgo)
{
    QList<int> digits;

    for (QChar _ch : strAlgo) {
        if (_ch.isDigit())
            digits.append(_ch.digitValue());
    }

    switch (digitsToNum(digits)) {
        case 1:
            return QCryptographicHash::Sha1;
        case 512:
            return QCryptographicHash::Sha512;
        default:
            return QCryptographicHash::Sha256;
    }
}

int digitsToNum(const QList<int> &digits)
{
    int number = 0;

    for (int _digit : digits) {
        number = (number * 10) + _digit;
    }

    return number;
}

bool canBeChecksum(const QString &str)
{
    if (str.length() != 40
        && str.length() != 64
        && str.length() != 128)
    {
        return false;
    }

    for (int i = 0; i < str.length(); ++i) {
        if (!str.at(i).isLetterOrNumber())
            return false;
    }

    return true;
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
        const int _ch1v = dt_before.at(i).digitValue();
        const int _ch2v = dt_later.at(i).digitValue();

        if (_ch1v < _ch2v)
            return true;
        if (_ch1v > _ch2v)
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

QString joinStrings(const QString &str1, const QString &str2, QChar sep)
{
    const bool s1Ends = str1.endsWith(sep);
    const bool s2Starts = str2.startsWith(sep);

    if (s1Ends && s2Starts) {
        QStringView _chopped = QStringView(str1).left(str1.size() - 1);
        return _chopped % str2;
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
QString basicName(const QString &path)
{
    if (isRoot(path)) {
        const QChar _ch = path.at(0);
        return _ch.isLetter() ? QStringLiteral(u"Drive_") + _ch.toUpper() : "Root";
    }

    // _sep == '/'
    const bool _endsWithSep = path.endsWith(_sep);
    const int _lastSepInd = path.lastIndexOf(_sep, -2);

    if (_lastSepInd == -1) {
        return _endsWithSep ? path.chopped(1) : path;
    }

    const int _len = _endsWithSep ? (path.size() - _lastSepInd - 2) : -1;
    return path.mid(_lastSepInd + 1, _len);
}

QString relativePath(const QString &rootFolder, const QString &fullPath)
{
    if (rootFolder.isEmpty())
        return fullPath;

    if (!fullPath.startsWith(rootFolder))
        return QString();

    // _sep == u'/';
    const int _cut = rootFolder.endsWith(_sep) ? rootFolder.size() - 1 : rootFolder.size();

    return ((_cut < fullPath.size()) && (fullPath.at(_cut) == _sep)) ? fullPath.mid(_cut + 1) : QString();

    // #2 impl. --> x2 slower due to (rootFolder + '/')
    // const QString &_root = rootFolder.endsWith('/') ? rootFolder : rootFolder + '/';
    // return fullPath.startsWith(_root) ? fullPath.mid(_root.size()) : QString();
}

QString shortenPath(const QString &path)
{
    return paths::isRoot(paths::parentFolder(path)) ? path
                                                    : QStringLiteral(u"../") + paths::basicName(path);
}

QString parentFolder(const QString &path)
{
    const int ind = path.lastIndexOf(_sep, -2);

    switch (ind) {
    case -1: // root --> root; string'/' --> ""
        return isRoot(path) ? path : QString();
    case 0: // /folder'/' --> "/"
        return path.at(ind);
    case 2: // C:/folder'/' --> "C:/"
        return isRoot(path.left(ind)) ? path.left(3) : path.left(ind);
    default: // /folder/item'/' --> /folder
        return path.left(ind);
    }
}

QString joinPath(const QString &absolutePath, const QString &addPath)
{
    return tools::joinStrings(absolutePath, addPath, _sep);
}

QString composeFilePath(const QString &parentFolder, const QString &fileName, const QString &ext)
{
    // with sep check
    // const QString _file = tools::joinStrings(fileName, ext, u'.');
    // return joinPath(parentFolder, _file);

    // no sep check
    return parentFolder % _sep % fileName % _dot % ext;
}

QString suffix(const QString &_file)
{
    const int _dotInd = _file.lastIndexOf(_dot);
    return (_dotInd > 0) ? _file.right(_file.size() - _dotInd - 1).toLower() : QString();
}

bool isRoot(const QString &path)
{
    switch (path.length()) {
    case 1:
        return (path.at(0) == _sep); // Linux FS root
    case 2:
    case 3:
        return (path.at(0).isLetter() && path.at(1) == u':'); // Windows drive root
    default:
        return false;
    }
}

bool hasExtension(const QString &file, const QString &ext)
{
    const int _dotInd = file.size() - ext.size() - 1;

    return ((_dotInd >= 0 && file.at(_dotInd) == _dot)
            && file.endsWith(ext, Qt::CaseInsensitive));
}

bool hasExtension(const QString &file, const QStringList &extensions)
{
    for (const QString &_ext : extensions) {
        if (hasExtension(file, _ext))
            return true;
    }

    return false;
}

bool isDbFile(const QString &filePath)
{
    return hasExtension(filePath, Lit::sl_db_exts);
}

bool isDigestFile(const QString &filePath)
{
    return hasExtension(filePath, Lit::sl_digest_exts);
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

    QChar _ch;
    switch (it) {
    case 1:
        _ch = u'K';
        break;
    case 2:
        _ch = u'M';
        break;
    case 3:
        _ch = u'G';
        break;
    case 4:
        _ch = u'T';
        break;
    default:
        _ch = u'?';
        break;
    }

    const float x = std::round(converted * 100) / 100;
    return QString::number(x, 'f', 2) % _ch % QStringLiteral(u"iB");
}

QString dataSizeReadableExt(const qint64 sizeBytes)
{
    return QString("%1 (%2 bytes)").arg(dataSizeReadable(sizeBytes), numString(sizeBytes));
}

QString shortenString(const QString &string, int length, bool cutEnd)
{
    if (string.length() <= length)
        return string;

    QString _dots = QStringLiteral(u"...");

    return cutEnd ? string.left(length).append(_dots)
                  : string.right(length).prepend(_dots);
}

QString simplifiedChars(QString str)
{
    /*
     * str = str.simplified();
     * static const QRegularExpression re("[ /\\\\:;%*?|<>&#~^]");
     * str.replace(re, "_");
     */

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

    const QString _folderStr = simplifiedChars(paths::basicName(folder));
    const QString _dbFileName = tools::joinStrings(prefix, _folderStr, u'_');

    return tools::joinStrings(_dbFileName, extension, u'.');
}

QString algoToStr(QCryptographicHash::Algorithm algo, bool capitalLetters)
{
    switch (algo) {
        case QCryptographicHash::Sha1:
            return capitalLetters ? QStringLiteral(u"SHA-1") : Lit::sl_digest_exts.at(0);
        case QCryptographicHash::Sha256:
            return capitalLetters ? QStringLiteral(u"SHA-256") : Lit::sl_digest_exts.at(1);
        case QCryptographicHash::Sha512:
            return capitalLetters ? QStringLiteral(u"SHA-512") : Lit::sl_digest_exts.at(2);
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

    const QString _files = (number == 1) ? QStringLiteral(u"file") : QStringLiteral(u"files");

    return tools::joinStrings(number, _files);
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

QString fileItemStatus(FileStatus status)
{
    switch (status) {
    case FileStatus::Queued: return QStringLiteral(u"queued");
    case FileStatus::Calculating: return QStringLiteral(u"calculating...");
    case FileStatus::Verifying: return QStringLiteral(u"verifying...");
    case FileStatus::NotChecked: return QStringLiteral(u"ready...");
    case FileStatus::NotCheckedMod: return QStringLiteral(u"ready... (modif.)");
    case FileStatus::Matched: return QStringLiteral(u"match");
    case FileStatus::Mismatched: return QStringLiteral(u"not match");
    case FileStatus::New: return QStringLiteral(u"new file");
    case FileStatus::Missing: return QStringLiteral(u"missing");
    case FileStatus::Added: return QStringLiteral(u"added");
    case Files::Removed: return QStringLiteral(u"removed");
    case FileStatus::Updated: return QStringLiteral(u"updated");
    case FileStatus::UnPermitted: return QStringLiteral(u"no permissions");
    case FileStatus::ReadError: return QStringLiteral(u"read error");
    default: return "unknown";
    }
}

QString coloredText(bool ignore)
{
    QString _color = ignore ? QStringLiteral(u"red") : QStringLiteral(u"green");
    return QStringLiteral(u"color : ") + _color;
}

QString coloredText(const QString &className, bool ignore)
{
    // "%1 { %2 }"
    return className % QStringLiteral(u" { ") % coloredText(ignore) % QStringLiteral(u" }");
}

} // namespace format
