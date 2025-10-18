/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef HASHER_H
#define HASHER_H

#include <QObject>
#include "QCryptographicHash"
#include "procstate.h"

class Hasher : public QObject
{
    Q_OBJECT

public:
    explicit Hasher(QObject *parent = nullptr);
    explicit Hasher(QCryptographicHash::Algorithm algo, QObject *parent = nullptr);
    void setAlgorithm(QCryptographicHash::Algorithm algo);
    void setProcState(const ProcState *procState);

    QString calculate(const QString &filePath);
    QString calculate(const QString &filePath, QCryptographicHash::Algorithm algo);

private:
    inline bool isCanceled() const;

    // file read buffer size
    int m_chunk = 1048576;

    QCryptographicHash::Algorithm m_algo = QCryptographicHash::Sha256;
    const ProcState *m_proc = nullptr;

signals:   
    void doneChunk(int done);
}; // class Hasher

#endif // HASHER_H
