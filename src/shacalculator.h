/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef SHACALCULATOR_H
#define SHACALCULATOR_H

#include <QObject>
#include "QCryptographicHash"
#include "procstate.h"

class ShaCalculator : public QObject
{
    Q_OBJECT
public:
    explicit ShaCalculator(QObject *parent = nullptr);
    explicit ShaCalculator(QCryptographicHash::Algorithm algo, QObject *parent = nullptr);
    void setAlgorithm(QCryptographicHash::Algorithm algo);
    void setProcState(const ProcState *procState);

public slots:
    QString calculate(const QString &filePath);
    QString calculate(const QString &filePath, QCryptographicHash::Algorithm algo);
    void cancelProcess();

private:
    int chunk = 1048576; // file read buffer size
    bool canceled = false; // if true, task should be aborted
    QCryptographicHash::Algorithm algo_ = QCryptographicHash::Sha256;
    const ProcState *proc_ = nullptr;

signals:   
    void doneChunk(int done);
};

#endif // SHACALCULATOR_H
