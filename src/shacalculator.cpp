/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "shacalculator.h"
#include <QFile>
#include <QDebug>

ShaCalculator::ShaCalculator(QObject *parent)
    : QObject(parent)
{}

ShaCalculator::ShaCalculator(QCryptographicHash::Algorithm algo, QObject *parent)
    : QObject(parent), algo_(algo)
{}

void ShaCalculator::setAlgorithm(QCryptographicHash::Algorithm algo)
{
    algo_ = algo;
}

void ShaCalculator::setProcState(const ProcState *procState)
{
    proc_ = procState;
}

QString ShaCalculator::calculate(const QString &filePath)
{
    return calculate(filePath, algo_);
}

QString ShaCalculator::calculate(const QString &filePath, QCryptographicHash::Algorithm algo)
{
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly)) {
        QCryptographicHash hash(algo);
        while (!file.atEnd() && !isCanceled()) {
            const QByteArray &buf = file.read(chunk);
            if (buf.size() > 0) {
                hash.addData(buf);
                emit doneChunk(buf.size());
            }
            else {
                qDebug() << "ShaCalculator::calculate >> ERROR:" << filePath;
                return QString();
            }
        }

        if (!isCanceled()) // result
            return hash.result().toHex();
    }

    return QString();
}

bool ShaCalculator::isCanceled() const
{
    return (proc_ && proc_->isCanceled());
}
