/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "tools.h"
#include <QDateTime>
#include <QFileInfo>
#include <cmath>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>
#include "files.h"

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

QCryptographicHash::Algorithm algorithmByStrLen(int strLen)
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
    const QString _str = strAlgo.right(3);
    QList<int> digits;

    for (QChar _ch : _str) {
        if (_ch.isDigit())
            digits.append(_ch.digitValue());
    }

    int number = 0;

    for (int digit : digits) {
        number = number * 10 + digit;
    }

    switch (number) {
        case 1:
            return QCryptographicHash::Sha1;
        case 512:
            return QCryptographicHash::Sha512;
        default:
            return QCryptographicHash::Sha256;
    }
}

bool isDatabaseFile(const QString &filePath) {
    return filePath.endsWith(QStringLiteral(u".ver"), Qt::CaseInsensitive)
           || filePath.endsWith(QStringLiteral(u".ver.json"), Qt::CaseInsensitive);
}

bool isSummaryFile(const QString &filePath)
{
    return (filePath.endsWith(QStringLiteral(u".sha1"), Qt::CaseInsensitive)
            || filePath.endsWith(QStringLiteral(u".sha256"), Qt::CaseInsensitive)
            || filePath.endsWith(QStringLiteral(u".sha512"), Qt::CaseInsensitive));
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

QString joinStrings(const QString &str1, const QString &str2, QChar sep)
{
    const bool s1Ends = str1.endsWith(sep);
    const bool s2Starts = str2.startsWith(sep);

    if (s1Ends && s2Starts)
        return str1.left(str1.length() - 1) + str2;

    if (s1Ends || s2Starts)
        return str1 + str2;

    return QString("%1%2%3").arg(str1, sep, str2);
}
} // namespace tools

namespace paths {
QString basicName(const QString &path)
{
    if (isRoot(path)) {
        const QChar _ch = path.at(0);
        return _ch.isLetter() ? QString("Drive_").append(_ch.toUpper()) : "Root";
    }

    QString result;
    const int lastIndex = path.size() - 1;

    for (int i = lastIndex; i >= 0; --i) {
        const QChar _ch = path.at(i);
        if (_ch != u'/') {
            result.prepend(_ch);
        }
        else if (i != lastIndex) {
            break;
        }
    }

    return result;

    /* prev. impl.
    static const QRegularExpression pathSep("[/\\\\]");
    const QStringList &components = path.split(pathSep, Qt::SkipEmptyParts);

    return components.isEmpty() ? QString() : components.last();
    */
}

QString relativePath(const QString &rootFolder, const QString &fullPath)
{
    if (rootFolder.isEmpty())
        return fullPath;

    if (!fullPath.startsWith(rootFolder))
        return QString();

    static const QChar _sep = '/';
    const int _cut = rootFolder.endsWith(_sep) ? rootFolder.size() - 1 : rootFolder.size();

    return ((_cut < fullPath.size()) && (fullPath.at(_cut) == _sep)) ? fullPath.mid(_cut + 1) : QString();

    // #2 impl. --> x2 slower due to (rootFolder + '/')
    // const QString &_root = rootFolder.endsWith('/') ? rootFolder : rootFolder + '/';
    // return fullPath.startsWith(_root) ? fullPath.mid(_root.size()) : QString();
}

QString shortenPath(const QString &path)
{
    return paths::isRoot(paths::parentFolder(path)) ? path
                                                    : "../" + paths::basicName(path);
}

QString parentFolder(const QString &path)
{
    const int ind = path.lastIndexOf('/', -2);

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

bool isRoot(const QString &path)
{
    switch (path.length()) {
    case 1:
        return (path.at(0) == '/'); // Linux FS root
    case 2:
    case 3:
        return (path.at(0).isLetter() && path.at(1) == ':'); // Windows drive root
    default:
        return false;
    }
}

QString joinPath(const QString &absolutePath, const QString &addPath)
{
    return tools::joinStrings(absolutePath, addPath, '/');
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
    return QDateTime::currentDateTime().toString("yyyy/MM/dd HH:mm");
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

    if (hours > 0)
        return approx ? QString("%1 h %2 min").arg(hours).arg(minutes)
                      : QString("%1 h %2 min %3 sec").arg(hours).arg(minutes).arg(seconds);

    if (approx && minutes > 0 && seconds > 15)
        return QString("%1 min").arg(minutes + 1);

    if (minutes > 0)
        return approx ? QString("%1 min").arg(minutes)
                      : QString("%1 min %2 sec").arg(minutes).arg(seconds);

    return (approx && seconds < 5) ? QString("few sec")
                                   : QString("%1 sec").arg(seconds);
}

QString dataSizeReadable(const qint64 sizeBytes)
{
    if (sizeBytes <= 1000)
        return QString("%1 bytes").arg(sizeBytes);

    long double converted = sizeBytes;
    int it = 0; // number of divisions

    do {
        converted /= 1024;
        ++it;
    } while (converted > 1000);

    QChar _ch;
    switch (it) {
    case 1:
        _ch = 'K';
        break;
    case 2:
        _ch = 'M';
        break;
    case 3:
        _ch = 'G';
        break;
    case 4:
        _ch = 'T';
        break;
    default:
        _ch = '?';
        break;
    }

    const float x = std::round(converted * 100) / 100;
    return QString("%1 %2iB").arg(QString::number(x, 'f', 2), _ch);
}

QString dataSizeReadableExt(const qint64 sizeBytes)
{
    return QString("%1 (%2 bytes)").arg(dataSizeReadable(sizeBytes), numString(sizeBytes));
}

QString shortenString(const QString &string, int length, bool cutEnd)
{
    if (string.length() <= length)
        return string;

    return cutEnd ? string.left(length).append("...")
                  : string.right(length).prepend("...");
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
            str[i] = '_';
        }
    }

    while (str.contains("__"))
        str.replace("__", "_");

    return str;
}

QString composeDbFileName(const QString &prefix, const QString &folder, const QString &extension)
{
    if (folder.isEmpty())
        return tools::joinStrings(prefix, extension, '.');

    const QString _folderStr = simplifiedChars(paths::basicName(folder));
    const QString _dbFileName = tools::joinStrings(prefix, _folderStr, '_');

    return tools::joinStrings(_dbFileName, extension, '.');
}

QString algoToStr(QCryptographicHash::Algorithm algo, bool capitalLetters)
{
    switch (algo) {
        case QCryptographicHash::Sha1:
            return capitalLetters ? "SHA-1" : "sha1";
        case QCryptographicHash::Sha256:
            return capitalLetters ? "SHA-256" : "sha256";
        case QCryptographicHash::Sha512:
            return capitalLetters ? "SHA-512" : "sha512";
        default:
            return "Unknown";
    }
}

QString algoToStr(int sumStrLength, bool capitalLetters)
{
    return algoToStr(tools::algorithmByStrLen(sumStrLength), capitalLetters);
}

QString filesNumberAndSize(int filesNumber, qint64 filesSize)
{
    if (filesNumber == 0)
        return QStringLiteral(u"no files");

    // if only 1 file the text is "file", if more the text is "files"
    const QString s = (filesNumber != 1) ? "s" : QString(); // null QChar is \u0000, so QString is used

    return QString("%1 file%2 (%3)")
                    .arg(filesNumber)
                    .arg(s, dataSizeReadable(filesSize));
}

QString fileNameAndSize(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    return QString("%1 (%2)").arg(fileInfo.fileName(), dataSizeReadable(fileInfo.size()));
}

QString fileItemStatus(FileStatus status)
{
    switch (status) {
    case FileStatus::Queued: return QStringLiteral(u"queued");
    case FileStatus::Calculating: return QStringLiteral(u"calculating...");
    case FileStatus::Verifying: return QStringLiteral(u"verifying...");
    case FileStatus::NotChecked: return QStringLiteral(u"ready...");
    case FileStatus::Matched: return QStringLiteral(u"match");
    case FileStatus::Mismatched: return QStringLiteral(u"not match");
    case FileStatus::New: return QStringLiteral(u"new file");
    case FileStatus::Missing: return QStringLiteral(u"missing");
    case FileStatus::Unreadable: return QStringLiteral(u"unreadable");
    case FileStatus::Added: return QStringLiteral(u"added");
    case Files::Removed: return QStringLiteral(u"removed");
    case FileStatus::Updated: return QStringLiteral(u"updated");
    default: return "unknown";
    }
}

QString coloredText(bool ignore)
{
    QString _color = ignore ? "red" : "green";
    return QString("color : %1").arg(_color);
}

QString coloredText(const QString &className, bool ignore)
{
    return QString("%1 { %2 }").arg(className, coloredText(ignore));
}

} // namespace format
