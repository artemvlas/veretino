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
    : QObject(parent), m_algo(algo)
{}

void ShaCalculator::setAlgorithm(QCryptographicHash::Algorithm algo)
{
    m_algo = algo;
}

void ShaCalculator::setProcState(const ProcState *procState)
{
    m_proc = procState;
}

QString ShaCalculator::calculate(const QString &filePath)
{
    return calculate(filePath, m_algo);
}

QString ShaCalculator::calculate(const QString &filePath, QCryptographicHash::Algorithm algo)
{
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly)) {
        QCryptographicHash hash(algo);
        while (!file.atEnd() && !isCanceled()) {
            const QByteArray &buf = file.read(m_chunk);
            if (buf.size() > 0) {
                hash.addData(buf);
                emit doneChunk(buf.size());
            } else {
                qWarning() << "ShaCalculator::calculate >> ERROR:" << filePath;
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
    return (m_proc && m_proc->isCanceled());
}
