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
#include <QRegularExpression>
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
    switch (QString(strAlgo.toLower().remove("sha").remove("-")).toInt()) {
        case 1:
            return QCryptographicHash::Sha1;
        case 512:
            return QCryptographicHash::Sha512;
        default:
            return QCryptographicHash::Sha256;
    }
}

bool isDatabaseFile(const QString &filePath) {
    return filePath.endsWith(".ver", Qt::CaseInsensitive)
           || filePath.endsWith(".ver.json", Qt::CaseInsensitive);
}

bool isSummaryFile(const QString &filePath)
{
    return (filePath.endsWith(".sha1", Qt::CaseInsensitive)
            || filePath.endsWith(".sha256", Qt::CaseInsensitive)
            || filePath.endsWith(".sha512", Qt::CaseInsensitive));
}

bool canBeChecksum(const QString &str)
{
    bool isOK = false;

    if (str.length() == 40 || str.length() == 64 || str.length() == 128) {
        isOK = true;
        for (int i = 0; isOK && i < str.length(); ++i) {
            if (!str.at(i).isLetterOrNumber())
                isOK = false;
        }
    }

    return isOK;
}

QStringList strToList(const QString &str)
{
    static const QRegularExpression re("[, ]");
    return str.split(re, Qt::SkipEmptyParts);
}
} // namespace tools

namespace paths {
QString basicName(const QString &path)
{
    if (isRoot(path)) {
        const QChar _ch = path.at(0);
        return _ch.isLetter() ? QString("Drive_%1").arg(_ch.toUpper()) : "Root";
    }

    static const QRegularExpression pathSep("[/\\\\]");
    const QStringList &components = path.split(pathSep, Qt::SkipEmptyParts);

    return components.isEmpty() ? QString() : components.last();

    // #2 impl. (for single '/' only)
    /*
    QString result = path.mid(path.lastIndexOf(QRegExp("[/\\\\]"), -2) + 1);
    if (result.endsWith('/') || result.endsWith("\\"))
        result.chop(1);
    return result;
    */
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
    return absolutePath.endsWith('/') ? absolutePath + addPath
                                      : QString("%1/%2").arg(absolutePath, addPath);
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
    /* old impl.
    str = str.simplified();

    QString forbSymb(" :/\\%*?|<>^&#");

    for (int i = 0; i < str.size(); ++i) {
        if (forbSymb.contains(str.at(i))) {
            str[i] = '_';
        }
    }*/

    static const QRegularExpression re("[ /\\\\:;%*?|<>&#~^]");
    str.replace(re, "_");

    while (str.contains("__"))
        str.replace("__", "_");

    return str;
}

QString joinStrings(const QString &str1, const QString &str2, const QString joint)
{
    if (str1.endsWith(joint) && str2.startsWith(joint))
        return str1.left(str1.length() - joint.length()) + str2;

    if (str1.endsWith(joint) || str2.startsWith(joint))
        return str1 + str2;

    return QString("%1%2%3").arg(str1, joint, str2);
}

QString composeDbFileName(const QString &prefix, const QString &folderName, const QString &extension)
{
    return folderName.isEmpty() ? joinStrings(prefix, extension, ".")
                                : joinStrings(joinStrings(prefix, simplifiedChars(paths::basicName(folderName)), "_"), extension, ".");
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
        return "no files";

    // if only 1 file the text is "file", if more the text is "files"
    const QString s = (filesNumber == 1) ? "file" : "files";

    return QString("%1 %2 (%3)")
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
    case FileStatus::Queued: return "queued";
    case FileStatus::Calculating: return "calculating...";
    case FileStatus::Verifying: return "verifying...";
    case FileStatus::NotChecked: return "ready...";
    case FileStatus::Matched: return "match";
    case FileStatus::Mismatched: return "not match";
    case FileStatus::New: return "new file";
    case FileStatus::Missing: return "missing";
    case FileStatus::Unreadable: return "unreadable";
    case FileStatus::Added: return "added";
    case Files::Removed: return "removed";
    case FileStatus::Updated: return "updated";
    default: return "unknown";
    }
}

QString coloredText(bool ignore)
{
    QString color = ignore ? "red" : "green";
    return QString("color : %1").arg(color);
}

QString coloredText(const QString &className, bool ignore)
{
    return QString("%1 { %2 }").arg(className, coloredText(ignore));
}

} // namespace format
