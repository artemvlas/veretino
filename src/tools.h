// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef TOOLS_H
#define TOOLS_H

#include <QString>
#include <QCryptographicHash>
#include "files.h"

struct Settings {
    QCryptographicHash::Algorithm algorithm = QCryptographicHash::Sha256;
    FilterRule filter;
    QString dbPrefix = "checksums";
};

namespace tools {
int algoStrLen(QCryptographicHash::Algorithm algo); // returns the length of checksum string depending on the sha-type: sha(1) = 40, sha(256) = 64, sha(512) = 128
QCryptographicHash::Algorithm algorithmByStrLen(int strLen); // ^vice versa
QCryptographicHash::Algorithm strToAlgo(const QString &strAlgo);

bool isDatabaseFile(const QString &filePath);
bool isSummaryFile(const QString &filePath);
bool canBeChecksum(const QString &text);
} // namespace tools

namespace format {
QString currentDateTime();

QString numString(qint64 num); // Returns a string of numbers separated by commas: 1,234,567,890
QString dataSizeReadable(qint64 sizeBytes); // converts size in bytes to human readable form like "129.17 GiB"
QString dataSizeReadableExt(qint64 sizeBytes); // returning style example: "6.08 GiB (6,532,974,324 bytes)"
QString shortenString(const QString &string, int length = 64);
QString algoToStr(QCryptographicHash::Algorithm algo);

QString fileNameAndSize(const QString &filePath); // returns "filename (readable size)" for file
QString filesNumberAndSize(int filesNumber, qint64 filesSize); // returns "number file's' (readable size)"

QString fileItemStatus(int status);
} // namespace format

namespace Mode {
enum Modes {
    NoMode,
    Folder,
    File,
    DbFile,
    SumFile,
    Model,
    ModelNewLost,
    UpdateMismatch,
    Processing,
    EndProcess
};
} // namespace Mode

#endif // TOOLS_H
