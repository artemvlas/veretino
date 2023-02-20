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
    QStringList filelist = mainData.keys();
    QStringList actualFiles = Files().actualFileListFiltered(workDir, ignoredExtensions); // all files from workDir except ignored extensions and *.ver.json and *.sha1/256/512

    foreach(const QString &i, filelist) {

        if (mainData[i] == "unreadable") {
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
        newlost.insert(fillMapSameValues(lostFiles, "LOST file"));
    }

    if (newFilesNumber > 0) {
        newlost.insert(fillMapSameValues(newFiles, "NEW file"));
    }

    return newlost;
}

QMap<QString,QString> DataContainer::clearDataFromLostFiles()
{
    if (lostFilesNumber > 0) {
        foreach (const QString &file, lostFiles) {
            mainData.remove(file);
        }

        QMap<QString,QString> changes = fillMapSameValues(lostFiles, "removed from DB");
        lostFiles.clear();
        lostFilesNumber = 0;
        qDebug()<< "DataContainer::clearDataFromLostFiles() | 'lostFiles' cleared";
        return changes;
    }
    else {
        qDebug() << "DataContainer::clearDataFromLostFiles() | NO Lost Files";
        return QMap<QString,QString>();
    }
}

QMap<QString,QString> DataContainer::updateMainData(const QMap<QString,QString> &listFilesChecksums, const QString &info)
{
    if (!listFilesChecksums.isEmpty()) {
        mainData.insert(listFilesChecksums); // update the mainData map with new data

        QMap<QString,QString> changes = fillMapSameValues(listFilesChecksums.keys(), info); // create 'changes' map to display a list of changes

        // determine and clear the origin of the added Data
        if (mismatches.contains(listFilesChecksums.firstKey())) {
            mismatches.clear();
            recalculated.clear();
            qDebug()<< "DataContainer::updateMainData() | mainData updated, 'mismatches' and 'recalculated' cleared";
        }
        else if (newFiles.contains(listFilesChecksums.firstKey())) {
            newFiles.clear();
            newFilesNumber = 0;
            qDebug()<< "DataContainer::updateMainData() | mainData updated, 'newFiles' cleared";
        }
        return changes;
    }
    else {
        qDebug() << "DataContainer::updateMainData() | NO Data to add";
        return QMap<QString,QString>();
    }
}

QMap<QString,QString> DataContainer::fillMapSameValues(const QStringList &keys, const QString &value)
{
    QMap<QString,QString> resultMap;

    foreach (const QString &key, keys) {
        resultMap.insert(key, value);
    }

    return resultMap;
}

DataContainer::~DataContainer()
{
    qDebug()<<"DataContainer deleted | " << this->objectName();
}
