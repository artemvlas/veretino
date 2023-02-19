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

    QMap<QString,QString> resultMap; // for writing to json
    QMap<QString,QString> parsedData;
    QMap<QString,QString> filesAvailability; //contains file : availability status (on Disk or Lost or New)
    QMap<QString,QString> differences; //files with failed checksum test
    QMap<QString,QString> recalculated; //checksums recalculated for given filelist (from json database) during verification
    QMap<QString,QString>& mapToSave();
    QStringList lostFiles;
    QStringList newFiles;
    QStringList onDiskFiles;

    QMap<QString,QString>& defineFilesAvailability();
    QMap<QString,QString> newlostOnly();

    int lostFilesNumber = 0;
    int newFilesNumber = 0;

    void clearNewLostFiles();

};

#endif // DATACONTAINER_H
