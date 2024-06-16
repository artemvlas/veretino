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
            //qDebug() << "tools::algoStrLen | Wrong input algo:" << algo;
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
            //qDebug() << "tools::algorithmByStrLen | Wrong input strLen:" << strLen;
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
    if (str.isEmpty())
        return QStringList();

    QString sep = " ";
    QStringList sepVariants { ", ", ",", "  " };

    for (int i = 0; i < sepVariants.size(); ++i) {
        if (str.contains(sepVariants.at(i))) {
            sep = sepVariants.at(i);
            break;
        }
    }

    return str.split(sep);
}
} // namespace tools

namespace paths {
QString basicName(const QString &path)
{
    if (path == "/")
        return "Root";

    if (path.size() == 3 && path.at(0).isLetter() && path.at(1) == ':') // if provided Windows-style root path like "C:"
        return QString("Drive_%1").arg(path.at(0).toUpper());

    // #1 impl.
    QChar pathSep = path.contains('\\') ? '\\' : '/';
    QStringList components = path.split(pathSep, Qt::SkipEmptyParts);
    return components.isEmpty() ? QString() : components.last();

    // #2 impl. (for single '/' only)
    /*
    QString result = path.mid(path.lastIndexOf(QRegExp("[/\\\\]"), -2) + 1);
    if (result.endsWith('/') || result.endsWith("\\"))
        result.chop(1);
    return result;
    */
}

QString parentFolder(const QString &path)
{
    if (path.isEmpty() || isRoot(path))
        return path;

    int rootSepIndex = path.indexOf('/'); // index of root '/': 0 for '/home/folder'; 2 for 'C:/folder'
    if (rootSepIndex == -1)
        return "/"; // if there is no '/' in 'path'

    // if the path's root contains double '/' like 'ftp://folder' or 'smb://folder', increase index to next position
    if (path.length() > rootSepIndex + 1
        && path.at(rootSepIndex + 1) == '/')
        ++rootSepIndex;

    int sepIndex = path.lastIndexOf('/', -2); // skip the last char due the case /home/folder'/'

    return (sepIndex > rootSepIndex) ? path.left(sepIndex)
                                     : path.left(rootSepIndex + 1); // if the last 'sep' is also the root, keep it
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

bool isRoot(const QString &path)
{
    return (path == "/") // Linux FS root
           || ((path.length() == 2 || path.length() == 3) // Windows drive root
               && path.at(0).isLetter() && path.at(1) == ':');
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

QString dataSizeReadable(qint64 sizeBytes)
{
    long double converted = sizeBytes;
    QString xB;

    if (converted > 1000) {
        converted /= 1024;
        xB = "KiB";
        if (converted > 1000) {
            converted /= 1024;
            xB = "MiB";
            if (converted > 1000) {
                converted /= 1024;
                xB = "GiB";
                if (converted > 1000) {
                    converted /= 1024;
                    xB = "TiB";
                }
            }
        }

        float x = std::round(converted * 100) / 100;
        return QString("%1 %2").arg(QString::number(x, 'f', 2), xB);
    }
    else
        return QString("%1 bytes").arg(sizeBytes);
}

QString dataSizeReadableExt(qint64 sizeBytes)
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
    str = str.simplified();

    QString forbSymb(" :*/\\%?|<>^&#");

    for (int i = 0; i < str.size(); ++i) {
        if (forbSymb.contains(str.at(i))) {
            str[i] = '_';
        }
    }

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

    QString s("files");

    if (filesNumber == 1)
        s = "file"; // if only 1 file the text is "file", if more the text is "files"

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
    QString result;

    switch (status) {
    case FileStatus::Queued:
        result = "queued";
        break;
    case FileStatus::Calculating:
        result = "calculating...";
        break;
    case FileStatus::Verifying:
        result = "verifying...";
        break;
    case FileStatus::NotChecked:
        result = "ready...";
        break;
    case FileStatus::Matched:
        result = "match";
        break;
    case FileStatus::Mismatched:
        result = "not match";
        break;
    case FileStatus::New:
        result = "new file";
        break;
    case FileStatus::Missing:
        result = "missing";
        break;
    case FileStatus::Unreadable:
        result = "ureadable";
        break;
    case FileStatus::Added:
        result = "added";
        break;
    case Files::Removed:
        result = "removed";
        break;
    case FileStatus::Updated:
        result = "updated";
        break;
    default:
        result = "unknown";
        break;
    }

    return result;
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
