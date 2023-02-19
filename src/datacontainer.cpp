#include "datacontainer.h"

DataContainer::DataContainer(const QString &initPath, QObject *parent)
    : QObject{parent}
{
    QFileInfo initPathInfo (initPath);

    if (initPathInfo.isFile() && initPath.endsWith(".ver.json")) {
        jsonFilePath = initPath;
        workDir = initPathInfo.absolutePath() + '/';
    }
    else if (initPathInfo.isDir()) {
        workDir = initPath;
        jsonFilePath = QString("%1/checksums_%2.ver.json").arg(workDir, QDir(workDir).dirName());
    }
    else {
        qDebug()<< "DataContainer | Wrong initPath" << initPath;
    }

    this->setObjectName(QString("DataContainer_%1").arg(initPathInfo.baseName()));

    qDebug()<<"DataContainer created | " << this->objectName();
}

QMap<QString,QString>& DataContainer::defineFilesAvailability()
{
    QStringList filelist = parsedData.keys();
    QStringList actualFiles = Files().actualFileListFiltered(workDir, ignoredExtensions); // all files from workDir except ignored extensions and *.ver.json and *.sha1/256/512

    foreach(const QString &i, filelist) {

        if (parsedData[i] == "unreadable") {
            if (QFileInfo::exists(i))
                filesAvailability[i] = "on Disk (unreadable)";
            else {
                filesAvailability[i] = "LOST file (unreadable)";
                lostFiles.append(i);
            }
        }

        else if (QFileInfo::exists(i)) {
            filesAvailability[i] = "on Disk";
            onDiskFiles.append(i);
        }
        else {
            filesAvailability[i] = "LOST file";
            lostFiles.append(i);
        }
    }

    foreach(const QString &i, actualFiles) {
        if(!filelist.contains(i)) {
            filesAvailability[i] = "NEW file";
            newFiles.append(i);
        }
    }

    lostFilesNumber = lostFiles.size();
    newFilesNumber = newFiles.size();

    return filesAvailability;
}

QMap<QString,QString> DataContainer::newlostOnly()
{
    QMap<QString,QString> newlost;

    if (lostFilesNumber > 0) {
        foreach (const QString &file, lostFiles) {
            newlost.insert(file, "LOST file");
        }
    }

    if (newFilesNumber > 0) {
        foreach (const QString &file, newFiles) {
            newlost.insert(file, "NEW file");
        }
    }

    return newlost;
}

void DataContainer::clearNewLostFiles()
{
    newFiles.clear();
    lostFiles.clear();
    newFilesNumber = 0;
    lostFilesNumber = 0;
}

QMap<QString,QString>& DataContainer::mapToSave()
{
    if (!resultMap.isEmpty())
        return resultMap;
    else
        return parsedData;
}

DataContainer::~DataContainer()
{
    qDebug()<<"DataContainer deleted | " << this->objectName();
}
