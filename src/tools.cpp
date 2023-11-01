// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "tools.h"
#include <QDateTime>
#include <QFileInfo>
#include <cmath>
#include <QDebug>
#include "files.h"

namespace tools {
int algoStrLen(QCryptographicHash::Algorithm algo)
{
    if (algo == QCryptographicHash::Sha1)
        return 40;
    else if (algo == QCryptographicHash::Sha256)
        return 64;
    else if (algo == QCryptographicHash::Sha512)
        return 128;
    else {
        qDebug() << "tools::algoStrLen | Wrong input algo:" << algo;
        return 0;
    }
}

QCryptographicHash::Algorithm algorithmByStrLen(int strLen)
{
    if (strLen == 40)
        return QCryptographicHash::Sha1;
    else if (strLen == 64)
        return QCryptographicHash::Sha256;
    else if (strLen == 128)
        return QCryptographicHash::Sha512;
    else {
        qDebug() << "tools::algorithmByStrLen | Wrong input strLen:" << strLen;
        return QCryptographicHash::Sha256;
    }
}

QCryptographicHash::Algorithm strToAlgo(const QString &strAlgo)
{
    int num = QString(strAlgo.toLower().remove("sha").remove("-")).toInt();
    if (num == 1)
        return QCryptographicHash::Sha1;
    else if (num == 512)
        return QCryptographicHash::Sha512;
    else
        return QCryptographicHash::Sha256;
}

bool isDatabaseFile(const QString &filePath) {
    return filePath.endsWith(".ver.json", Qt::CaseInsensitive);
}

bool isSummaryFile(const QString &filePath)
{
    return (filePath.endsWith(".sha1", Qt::CaseInsensitive)
            || filePath.endsWith(".sha256", Qt::CaseInsensitive)
            || filePath.endsWith(".sha512", Qt::CaseInsensitive));
}

bool canBeChecksum(const QString &text)
{
    bool isOK = false;

    if (text.length() == 40 || text.length() == 64 || text.length() == 128) {
        isOK = true;
        for (int i = 0; isOK && i < text.length(); ++i) {
            if (!text.at(i).isLetterOrNumber())
                isOK = false;
        }
    }

    return isOK;
}
} // namespace tools

namespace paths {
QString basicName(const QString &path)
{
    if (path == "/")
        return "Root";

    if (path.size() == 3 && path.at(0).isLetter() && path.at(1) == ':') // if Windows-style root path like C:
        return QString("Drive_%1").arg(path.at(0));

    // #1 impl.
    QStringList components = path.split(QRegExp("[/\\\\]"), Qt::SkipEmptyParts);
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
    int rootSepIndex = path.indexOf('/'); // index of root '/': 0 for '/home/folder'; 2 for 'C:/folder'
    if (rootSepIndex == -1)
        return "/"; // if there is no '/' in 'path'

    if (path.length() > rootSepIndex + 1 && path.at(rootSepIndex + 1) == '/')
        ++rootSepIndex; // if the path's root contains double '/' like 'ftp://folder' or 'smb://folder', increase index to next position
    int sepIndex = path.lastIndexOf('/', -2); // skip the last char due the case /home/folder'/'

    return (sepIndex > rootSepIndex) ? path.left(sepIndex) : path.left(rootSepIndex + 1); // if the last 'sep' is also the root, keep it
}

QString joinPath(const QString &absolutePath, const QString &addPath)
{
    if (absolutePath.endsWith('/'))
        return absolutePath + addPath;
    else
        return QString("%1/%2").arg(absolutePath, addPath);
}

QString backupFilePath(const QString &filePath)
{
    return joinPath(parentFolder(filePath), ".tmp-backup_" + paths::basicName(filePath));
}

bool isFileAllowed(const QString &filePath, const FilterRule &filter)
{
    if (!filter.includeOnly) {
        if (tools::isDatabaseFile(filePath))
            return !filter.ignoreDbFiles;
        if (tools::isSummaryFile(filePath))
            return !filter.ignoreShaFiles;
    }

    if (filter.extensionsList.isEmpty())
        return true;

    // if 'filter.include' = true, a file ('filePath') with any extension from 'extensionsList' is allowed
    // if 'filter.include' = false, than all files except these types allowed

    bool allowed = !filter.includeOnly;
    foreach (const QString &ext, filter.extensionsList) {
        if (filePath.endsWith('.' + ext, Qt::CaseInsensitive)) {
            allowed = filter.includeOnly;
            break;
        }
    }

    return allowed;
}
} // namespace paths

namespace ModelKit {
QString getPath(const QModelIndex &curIndex)
{
    QString path;
    QModelIndex newIndex = siblingAtRow(curIndex, PathColumn);

    if (newIndex.isValid()) {
        path = newIndex.data().toString();

        while (newIndex.parent().isValid()) {
            path = paths::joinPath(newIndex.parent().data().toString(), path);
            newIndex = newIndex.parent();
        }
    }

    return path;
}

QModelIndex getIndex(const QString &path, const QAbstractItemModel *model)
{
    QModelIndex curIndex;

    if (!path.isEmpty()) {
        QModelIndex parentIndex;
        curIndex = model->index(0, 0);
        QStringList parts = path.split('/');

        foreach (const QString &str, parts) {
            for (int i = 0; curIndex.isValid(); ++i) {
                curIndex = model->index(i, 0, parentIndex);
                if (curIndex.data().toString() == str) {
                    //qDebug() << "***" << str << "finded on" << i << "row";
                    parentIndex = model->index(i, 0, parentIndex);
                    break;
                }
                //qDebug() << "*** Looking for:" << str << curIndex.data();
            }
        }
        //qDebug() << "View::pathToIndex" << path << "-->" << curIndex << curIndex.data();
    }

    return curIndex;
}

QModelIndex siblingAtRow(const QModelIndex &curIndex, Columns column)
{
    return curIndex.isValid() ? curIndex.model()->index(curIndex.row(), column, curIndex.parent()) : QModelIndex();
}

// the TreeModel implies that if an item has children, then it is a folder, if not, then it is a file
bool isFileRow(const QModelIndex &curIndex)
{
    QModelIndex index = siblingAtRow(curIndex, PathColumn);

    return (index.isValid() && !index.model()->hasChildren(index));
}
} // namespace ModelKit

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

QString shortenString(const QString &string, int length)
{
    if (string.length() > length)
        return string.mid(0, length).append("...");
    else
        return string;
}

QString algoToStr(QCryptographicHash::Algorithm algo)
{
    if (algo == QCryptographicHash::Sha1)
        return "SHA-1";
    else if (algo == QCryptographicHash::Sha256)
        return "SHA-256";
    else if (algo == QCryptographicHash::Sha512)
        return "SHA-512";
    else
        return "Unknown";
}

QString filesNumberAndSize(int filesNumber, qint64 filesSize)
{
    if (filesNumber == 0)
        return "no files";

    QString s("files");

    if (filesNumber == 1)
        s = "file"; // if only 1 file the text is "file", if more the text is "files"

    return QString("%1 %2 (%3)").arg(filesNumber).arg(s, dataSizeReadable(filesSize));
}

QString fileNameAndSize(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    return QString("%1 (%2)").arg(fileInfo.fileName(), dataSizeReadable(fileInfo.size()));
}

QString fileItemStatus(int status)
{
    QString result;
    switch (status) {
    case FileValues::NotChecked:
        result = "ready...";
        break;
    case FileValues::Matched:
        result = "✓ OK";
        break;
    case FileValues::Mismatched:
        result = "☒ NOT match";
        break;
    case FileValues::New:
        result = "new file";
        break;
    case FileValues::Missing:
        result = "missing";
        break;
    case FileValues::Unreadable:
        result = "ureadable";
        break;
    case FileValues::Added:
        result = "→ added to DB"; // ➔
        break;
    case FileValues::Removed:
        result = "✂ removed from DB";
        break;
    case FileValues::ChecksumUpdated:
        result = "↻ stored checksum updated"; // 🗘
        break;
    default:
        result = "unknown";
    }

    return result;
}
} // namespace format
