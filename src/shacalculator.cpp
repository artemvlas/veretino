#include "shacalculator.h"
#include "tools.h"
#include <QFile>
#include <QFileInfo>

ShaCalculator::ShaCalculator(QObject *parent)
    : QObject(parent)
{
    connect(this, &ShaCalculator::cancelProcess, this, [&]{canceled = true;});
}

ShaCalculator::ShaCalculator(QCryptographicHash::Algorithm algo, QObject *parent)
    : QObject(parent)
{
    initAlgo = algo;

    connect(this, &ShaCalculator::cancelProcess, this, [&]{canceled = true;});
}

FileValues ShaCalculator::computeChecksum(const QString &filePath, QCryptographicHash::Algorithm algo)
{
    FileValues curFileValues;
    QFile file(filePath);
    if (!file.open (QIODevice::ReadOnly)) {
        curFileValues.status = FileValues::Unreadable;
        return curFileValues;
    }

    QCryptographicHash hash(algo);
    while (!file.atEnd() && !canceled) {
        const QByteArray &buf = file.read(chunk);
        hash.addData(buf);
        toPercents(buf.size()); // add this processed piece, calculate total done size and emit donePercents()
    }

    if (canceled) {        
        emit donePercents(0);
        qDebug() << "ShaCalculator::computeChecksum | Canceled";
    }
    else
        curFileValues.checksum = hash.result().toHex();

    return curFileValues;
}

QString ShaCalculator::calculate(const QString &filePath)
{
    return calculate(filePath, initAlgo);
}

QString ShaCalculator::calculate(const QString &filePath, QCryptographicHash::Algorithm algo)
{
    doneSize = 0;
    totalSize = QFileInfo(filePath).size();

    canceled = false;

    emit setStatusbarText(QString("Calculating %1 checksum: %2").arg(format::algoToStr(algo), Files(filePath).contentStatus()));

    FileValues curFileValues = computeChecksum(filePath, algo);

    if (canceled) {
        emit setStatusbarText("Canceled");
    }
    else if (curFileValues.status != FileValues::Unreadable && !curFileValues.checksum.isEmpty()) {
        emit setStatusbarText(QString("%1 calculated").arg(format::algoToStr(algo)));
    }
    else {
        emit setStatusbarText("read error");
    }

    return curFileValues.checksum;
}

FileList ShaCalculator::calculate(const DataContainer &filesContainer)
{
    FileList resultList;

    if (filesContainer.filesData.isEmpty())
        return resultList;

    canceled = false;
    totalSize = Files::dataSize(filesContainer.filesData);
    QString totalSizeReadable = format::dataSizeReadable(totalSize);
    doneSize = 0;
    FileList::const_iterator iter;

    for (iter = filesContainer.filesData.constBegin(); iter != filesContainer.filesData.constEnd() && !canceled; ++iter) {
        QString doneData;
        if (doneSize == 0)
            doneData = QString("(%1)").arg(totalSizeReadable);
        else
            doneData = QString("(%1 / %2)").arg(format::dataSizeReadable(doneSize), totalSizeReadable);

        emit setStatusbarText(QString("Calculating %1 of %2 checksums %3")
                        .arg(resultList.size() + 1)
                        .arg(filesContainer.filesData.size())
                        .arg(doneData));

        FileValues curFileValues = computeChecksum(paths::joinPath(filesContainer.metaData.workDir, iter.key()), filesContainer.metaData.algorithm);

        curFileValues.size = iter.value().size;
        resultList.insert(iter.key(), curFileValues);
    }

    if (canceled) {
        qDebug() << "ShaCalculator::calculate | Canceled";
        emit setStatusbarText("Canceled");
        return FileList();
    }

    emit setStatusbarText("Done");
    return resultList;
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
