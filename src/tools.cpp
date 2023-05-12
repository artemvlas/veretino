#include "tools.h"
#include <QDateTime>
#include <QFileInfo>
#include <cmath>
#include <QDebug>

namespace tools {
int shaStrLen(int shatype)
{
    if (shatype == 1)
        return 40;
    else if (shatype == 256)
        return 64;
    else if (shatype == 512)
        return 128;
    else {
        qDebug() << "tools::shaStrLen | Wrong input shatype:" << shatype;
        return 0;
    }
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

QString filesNumberAndSize(int filesNumber, qint64 filesSize)
{
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

} // namespace 'format'