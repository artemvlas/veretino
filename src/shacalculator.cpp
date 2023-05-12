#include "shacalculator.h"
#include <QFile>
#include <QFileInfo>

ShaCalculator::ShaCalculator(QObject *parent)
    : QObject(parent)
{
    connect(this, &ShaCalculator::cancelProcess, this, [&]{canceled = true;});
}

ShaCalculator::ShaCalculator(int shatype, QObject *parent)
    : QObject(parent)
{
    initShaType = shatype;

    connect(this, &ShaCalculator::cancelProcess, this, [&]{canceled = true;});
}

QCryptographicHash::Algorithm ShaCalculator::algorithm()
{
    return algorithm(initShaType);
}

QCryptographicHash::Algorithm ShaCalculator::algorithm(int shatype)
{
    if (shatype == 1)
        return QCryptographicHash::Sha1;
    else if (shatype == 512)
        return QCryptographicHash::Sha512;
    else
        return QCryptographicHash::Sha256;
}

FileValues ShaCalculator::computeChecksum(const QString &filePath, int shatype)
{
    FileValues curFileValues;
    QFile file(filePath);
    if (!file.open (QIODevice::ReadOnly)) {
        curFileValues.isReadable = false;
        return curFileValues;
    }

    QCryptographicHash hash(algorithm(shatype));
    while (!file.atEnd() && !canceled) {
        const QByteArray &buf = file.read(chunk);
        hash.addData(buf);
        toPercents(buf.size()); // add this processed piece, calculate total done size and emit donePercents()
    }

    if (canceled) {        
        emit donePercents(0);
        qDebug() << "ShaCalculator::computeChecksum | Canceled";
        curFileValues.about = "Canceled";
    }
    else
        curFileValues.checksum = hash.result().toHex();

    return curFileValues;
}

QString ShaCalculator::calculate(const QString &filePath)
{
    return calculate(filePath, initShaType);
}

QString ShaCalculator::calculate(const QString &filePath, int shatype)
{
    doneSize = 0;
    totalSize = QFileInfo(filePath).size();

    canceled = false;

    emit status(QString("Calculating SHA-%1 checksum: %2").arg(shatype).arg(Files(filePath).contentStatus()));

    FileValues curFileValues = computeChecksum(filePath, shatype);

    if (canceled) {
        emit status("Canceled");
    }
    else if (curFileValues.isReadable && !curFileValues.checksum.isEmpty()) {
        emit status(QString("SHA-%1 calculated").arg(shatype));
    }
    else {
        emit status("read error");
    }

    return curFileValues.checksum;
}

FileList ShaCalculator::calculate(const DataContainer &filesContainer)
{
    emit status("Total size calculating...");
    canceled = false;

    totalSize = Files::dataSize(filesContainer.filesData);
    doneSize = 0;

    QString totalInfo = format::filesNumberAndSize(filesContainer.filesData.size(), totalSize);

    emit status(QString("Checksums calculating: %1").arg(totalInfo));

    FileList resultList;
    FileList::const_iterator iter;
    for (iter = filesContainer.filesData.constBegin(); iter != filesContainer.filesData.constEnd() && !canceled; ++iter) {
        FileValues curFileValues = computeChecksum(paths::joinPath(filesContainer.metaData.workDir, iter.key()), filesContainer.metaData.shaType);
        if (!filesContainer.metaData.about.isEmpty())
            curFileValues.about = filesContainer.metaData.about;

        curFileValues.size = iter.value().size;

        resultList.insert(iter.key(), curFileValues);

        emit status(QString("Checksums calculating: done %1 (%2) of %3")
                        .arg(resultList.size())
                        .arg(format::dataSizeReadable(doneSize), totalInfo));
    }

    if (canceled) {
        qDebug() << "ShaCalculator::calculate | Canceled";
        emit status("Canceled");
        return FileList();
    }
    else {
        emit status("Done");
        return resultList;
    }
}

void ShaCalculator::toPercents(int bytes)
{
    if (doneSize == 0)
        emit donePercents(0); // initial 0 to reset progressbar value

    int lastPerc = (doneSize * 100) / totalSize; // before current chunk added

    doneSize += bytes;
    int curPerc = (doneSize * 100) / totalSize; // after

    if (curPerc > lastPerc) {
        emit donePercents(curPerc);
    }
}

ShaCalculator::~ShaCalculator()
{
    qDebug() << "ShaCalculator Destructed";
}
