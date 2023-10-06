// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "shacalculator.h"
#include <QFile>
#include <QDebug>

ShaCalculator::ShaCalculator(QCryptographicHash::Algorithm algo, QObject *parent)
    : QObject(parent), initAlgo(algo)
{}

QString ShaCalculator::calculate(const QString &filePath)
{
    return calculate(filePath, initAlgo);
}

QString ShaCalculator::calculate(const QString &filePath, QCryptographicHash::Algorithm algo)
{
    QString result;
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly)) {
        QCryptographicHash hash(algo);
        while (!file.atEnd() && !canceled) {
            const QByteArray &buf = file.read(chunk);
            hash.addData(buf);
            emit doneChunk(buf.size());
        }

        if (canceled)
            qDebug() << "ShaCalculator::calculate | Canceled";
        else
            result = hash.result().toHex();
    }

    return result;
}

void ShaCalculator::cancelProcess()
{
    canceled = true;
}
