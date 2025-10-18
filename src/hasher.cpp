/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "hasher.h"
#include "tools.h"
#include <QFile>

Hasher::Hasher(QObject *parent)
    : QObject(parent)
{}

Hasher::Hasher(QCryptographicHash::Algorithm algo, QObject *parent)
    : QObject(parent), m_algo(algo)
{}

void Hasher::setAlgorithm(QCryptographicHash::Algorithm algo)
{
    m_algo = algo;
}

void Hasher::setProcState(const ProcState *procState)
{
    m_proc = procState;
}

QString Hasher::calculate(const QString &filePath)
{
    return calculate(filePath, m_algo);
}

QString Hasher::calculate(const QString &filePath, QCryptographicHash::Algorithm algo)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        throw file.exists() ? ERR_NOPERM : ERR_NOTEXIST;
    }

    QCryptographicHash hash(algo);

    while (!file.atEnd() && !isCanceled()) {
        const QByteArray &buf = file.read(m_chunk);

        if (buf.size() > 0) {
            hash.addData(buf);
            emit doneChunk(buf.size());
        } else {
            throw ERR_READ;
        }
    }

    if (isCanceled())
        throw ERR_CANCELED;

    // result
    return hash.result().toHex();
}

bool Hasher::isCanceled() const
{
    return (m_proc && m_proc->isCanceled());
}
