// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef SHACALCULATOR_H
#define SHACALCULATOR_H

#include <QObject>
#include "QCryptographicHash"

class ShaCalculator : public QObject
{
    Q_OBJECT
public:
    explicit ShaCalculator(QCryptographicHash::Algorithm algo = QCryptographicHash::Sha256, QObject *parent = nullptr);

public slots:
    QString calculate(const QString &filePath);
    QString calculate(const QString &filePath, QCryptographicHash::Algorithm algo);
    void cancelProcess();

private:
    int chunk = 1048576; // file read buffer size
    bool canceled = false; // if true, task should be aborted
    QCryptographicHash::Algorithm initAlgo;

signals:   
    void doneChunk(int done);
};

#endif // SHACALCULATOR_H
