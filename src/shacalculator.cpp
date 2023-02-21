#include "shacalculator.h"
#include "qthread.h"

ShaCalculator::ShaCalculator(const int &shatype, QObject *parent)
    : QObject(parent)
{
    chunk = 1048576;

    if (shatype != 0)
        setShaType(shatype);

    canceled = false;
    qDebug()<< "ShaCalculator created: " << QThread::currentThread();
}

void ShaCalculator::setShaType(const int &shatype)
{
    if (shatype == 1)
        algorithm = QCryptographicHash::Sha1;
    else if (shatype == 256)
        algorithm = QCryptographicHash::Sha256;
    else if (shatype == 512)
        algorithm = QCryptographicHash::Sha512;
    else
        qDebug()<<"Wrong shatype. It must be int 1, 256 or 512";
}

QString ShaCalculator::calcSha (const QString &filePath)
{
    QFile file (filePath);
    if (!file.open (QIODevice::ReadOnly)) {
        return "unreadable";
    }

    QCryptographicHash hash (algorithm);
    while (!file.atEnd() && !canceled) {
        const QByteArray &buf = file.read(chunk);
        hash.addData (buf);

        toPercents(buf.size()); // add this processed piece, calculate total done size and emit donePercents()
    }

    if (canceled) {        
        emit donePercents(0);
        qDebug()<<"ShaCalculator::calcSha | Canceled";
        return QString();
    }
    else
        return hash.result().toHex();
}

QString ShaCalculator::calcShaFile (const QString &filePath, const int &shatype)
{
    if (shatype != 0)
        setShaType(shatype);

    doneSize = 0;
    totalSize = QFileInfo(filePath).size();

    canceled = false;
    QString sum = calcSha(filePath);
    //emit status(QString("SHA-%1: %2").arg(shatype).arg(sum));

    return sum;
}

QMap<QString,QString> ShaCalculator::calcShaList (const QStringList &filelist, const int &shatype)
{
    //emit status("Processing");
    if (shatype != 0)
        setShaType(shatype);

    QMap<QString,QString> map;
    int filesNumber = filelist.size();
    doneSize = 0;
    totalSize = Files().filelistSize(filelist);
    QString totalInfo = Files().filesNumberSizeToReadable(filesNumber, totalSize);
    canceled = false;
    QLocale locale (QLocale::English);

    for (int var = 0; var < filesNumber && !canceled; ++var) {
        emit status(QString("Calculation SHA-%1 checksums: done %2 (%3) of %4").arg(shatype).arg(var).arg(locale.formattedDataSize(doneSize), totalInfo));
        map[filelist.at(var)] = calcSha(filelist.at(var));
    }

    if(canceled) {
        qDebug()<<"ShaCalculator::calcShaList | Canceled";
        emit status("Canceled");
        return QMap<QString,QString> ();
    }
    else {
        //emit resultReady(map);
        emit status("Done");
        return map;
    }
}

void ShaCalculator::toPercents(const int &bytes)
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

void ShaCalculator::cancelProcess()
{
    canceled = true;
}

ShaCalculator::~ShaCalculator()
{
    qDebug()<< "ShaCalculator Destructed: " << QThread::currentThread();
}
