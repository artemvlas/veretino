/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
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
    if (!proc_)
        return QString();

    QString result;
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly)) {
        QCryptographicHash hash(algo);
        while (!file.atEnd() && !proc_->isCanceled()) {
            const QByteArray &buf = file.read(chunk);
            hash.addData(buf);
            emit doneChunk(buf.size());
        }

        if (proc_->isCanceled())
            qDebug() << "ShaCalculator::calculate | Canceled";
        else
            result = hash.result().toHex();
    }

    return result;
}
