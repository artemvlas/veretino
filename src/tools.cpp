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
    case FileValues::Matched:
        result = "✓ OK";
        break;
    case FileValues::Mismatched:
        result = "☒ NOT match";
        break;
    case FileValues::ChecksumUpdated:
        result = "↻ stored checksum updated"; // 🗘
        break;
    case FileValues::Added:
        result = "→ added to DB"; // ➔
        break;
    case FileValues::Removed:
        result = "✂ removed from DB";
        break;
    }
    return result;
}
} // namespace 'format'
