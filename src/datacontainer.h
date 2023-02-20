#ifndef DATACONTAINER_H
#define DATACONTAINER_H

#include <QObject>
#include <QFileInfo>
#include <QDir>
#include "files.h"
#include <QMap>
#include <QDebug>

/*
Objects of this class are used to store, organize, manage database data.
For each database, a separate object is created that stores checksums, all lists of files for various needs,
info about the algorithm type, relevance, etc.
The object can perform basic tasks of sorting, filtering, comparing data.
*/

class DataContainer : public QObject
{
    Q_OBJECT
public:
    explicit DataContainer(const QString &initPath = QString(), QObject *parent = nullptr);
    ~DataContainer();

    QString jsonFilePath;
    QString workDir; // current working folder with '/': /home/user/dataFolder/
    QStringList ignoredExtensions;
    int dbShaType = 0; // 1 or 256 or 512: from json database header or by checksum lenght
    QString lastUpdate; // from "Updated" value of first json object (from header)
    QString storedDataSize; // total size of listed files when db was built
    QMap<QString,QString> mainData;
    QMap<QString,QString> filesAvailability; //contains file : availability status (on Disk or Lost or New)
    QMap<QString,QString> mismatches; //files with failed checksum test
    QMap<QString,QString> recalculated; //checksums recalculated for given filelist (from json database) during verification
    QMap<QString,QString> fillMapSameValues(const QStringList &keys, const QString &value); // create the QMap with multiple keys(QStrinList) and same values
    QStringList lostFiles;
    QStringList newFiles;
    int lostFilesNumber = 0;
    int newFilesNumber = 0;
    QStringList onDiskFiles;

    QMap<QString,QString>& defineFilesAvailability();
    QMap<QString,QString> newlostOnly();
    QMap<QString,QString> clearDataFromLostFiles(); // remove lostFiles items from mainData, returns the list of changes
    QMap<QString,QString> updateMainData(const QMap<QString,QString> &listFilesChecksums, const QString &info = "added to DB"); // add calculated checksums to mainData, returns the list of changes

    void setJsonFileNamePrefix(const QString &prefix);
};

#endif // DATACONTAINER_H
