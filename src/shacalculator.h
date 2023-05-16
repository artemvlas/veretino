// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef SHACALCULATOR_H
#define SHACALCULATOR_H

#include <QObject>
#include "QCryptographicHash"
#include "jsondb.h"
#include "files.h"

class ShaCalculator : public QObject
{
    Q_OBJECT
public:
    explicit ShaCalculator(QObject *parent = nullptr);
    explicit ShaCalculator(int shatype, QObject *parent = nullptr);
    ~ShaCalculator();

public slots:
    QString calculate(const QString &filePath);
    QString calculate(const QString &filePath, int shatype);
    FileList calculate(const DataContainer &filesContainer);

private:
    int chunk = 1048576; // file read buffer size
    bool canceled = false; // if true, task should be aborted
    int initShaType = 0;
    qint64 totalSize; // total file or filelist size
    qint64 doneSize;
    QCryptographicHash::Algorithm algorithm();
    QCryptographicHash::Algorithm algorithm(int shatype);
    FileValues computeChecksum(const QString &filePath, int shatype);
    void toPercents(int bytes); // add this processed piece, calculate total done size and emit donePercents()

signals:
    void cancelProcess();
    void donePercents(int done);
    void status(const QString &text); // text to statusbar
};

#endif // SHACALCULATOR_H
