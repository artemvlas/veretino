// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef TOOLS_H
#define TOOLS_H

#include <QString>

namespace format {
QString currentDateTime();

QString numString(qint64 num); // Returns a string of numbers separated by commas: 1,234,567,890
QString dataSizeReadable(qint64 sizeBytes); // converts size in bytes to human readable form like "129.17 GiB"
QString dataSizeReadableExt(qint64 sizeBytes); // returning style example: "6.08 GiB (6,532,974,324 bytes)"

QString fileNameAndSize(const QString &filePath); // returns "filename (readable size)" for file
QString filesNumberAndSize(int filesNumber, qint64 filesSize); // returns "number file's' (readable size)"
} // namespace format

#endif // TOOLS_H
