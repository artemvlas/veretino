/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef TOOLS_H
#define TOOLS_H

#include <QString>
#include <QCryptographicHash>
#include <QAbstractItemModel>
#include <QPalette>
#include "files.h"

struct Settings {
    QCryptographicHash::Algorithm algorithm = QCryptographicHash::Sha256;
    FilterRule filter;
    QStringList recentFiles;
    QString dbPrefix = "checksums";
    bool restoreLastPathOnStartup = true;
    bool addWorkDirToFilename = true;
    bool isLongExtension = true;
    bool saveVerificationDateTime = false;

    QString dbFileExtension() const
    {
        return dbFileExtension(isLongExtension);
    }

    static QString dbFileExtension(bool isLong)
    {
        return isLong ? ".ver.json" : ".ver";
    }

    void addRecentFile(const QString &filePath)
    {
        if (!recentFiles.contains(filePath))
            recentFiles.prepend(filePath);
        else if (recentFiles.size() > 1 && recentFiles.first() != filePath) { // move the recent file to the top
            //recentFiles.move(recentFiles.indexOf(filePath), 0);

            for (int i = 1; i < recentFiles.size(); ++i) {
                if (recentFiles.at(i) == filePath) {
                    recentFiles.move(i, 0);
                    break;
                }
            }
        }

        if (recentFiles.size() > 10)
            recentFiles.removeLast();
    }
};

namespace tools {
int algoStrLen(QCryptographicHash::Algorithm algo); // returns the length of checksum string depending on the sha-type: sha(1) = 40, sha(256) = 64, sha(512) = 128
QCryptographicHash::Algorithm algorithmByStrLen(int strLen); // ^vice versa
QCryptographicHash::Algorithm strToAlgo(const QString &strAlgo);

QString findCompleteString(const QStringList &strList, const QString &sample, int sampleLength = 4); // strList{"Ignored", "Included"} | sample "ignore" --> "Ignored", sample "Include Only" --> "Included"

bool isDatabaseFile(const QString &filePath);
bool isSummaryFile(const QString &filePath);
bool canBeChecksum(const QString &text);
} // namespace tools

namespace paths {
QString parentFolder(const QString &path); // returns the parent folder of the 'path'
QString basicName(const QString &path); // returns file or folder name: "/home/user/folder/fname" --> "fname"
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
} // namespace format

#endif // TOOLS_H
