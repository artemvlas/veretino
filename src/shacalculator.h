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
    explicit ShaCalculator(const int &shatype = 0, QObject *parent = nullptr);
    ~ShaCalculator();

    void setShaType(const int &shatype);

public slots:
    QString calculateSha(const QString &filePath, const int &shatype = 0);
    QMap<QString,QString> calculateSha(const QStringList &filelist, const int &shatype = 0);

private:
    int chunk = 1048576; // file read buffer size
    bool canceled = false; // if true, task should be aborted
    qint64 totalSize; // total file or filelist size
    qint64 doneSize;
    QCryptographicHash::Algorithm algorithm;
    QString calcSha (const QString &filePath);
    void toPercents(const int &bytes); // add this processed piece, calculate total done size and emit donePercents()

signals:
    void cancelProcess();
    void status(const QString &status); //text to statusbar
    void resultReady(const QMap<QString,QString> &result);
    void donePercents(const int &done);
    void errorMessage(const QString &text);
};

#endif // SHACALCULATOR_H
