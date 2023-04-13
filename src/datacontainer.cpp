#include "datacontainer.h"
#include "files.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>

DataContainer::DataContainer(const QString &initPath, QObject *parent)
    : QObject{parent}
{
    QFileInfo initPathInfo(initPath);

    if (initPathInfo.isFile() && initPath.endsWith(".ver.json", Qt::CaseInsensitive)) {
        jsonFilePath = initPath;
        workDir = Files::parentFolder(initPath);
    }
    else if (initPathInfo.isDir()) {
        workDir = initPath;
        jsonFilePath = Files::joinPath(workDir, QString("checksums_%1.ver.json").arg(Files::folderName(workDir)));
    }
    else {
        qDebug() << "DataContainer | Wrong initPath" << initPath;
    }

    this->setObjectName(QString("DataContainer_%1").arg(initPathInfo.baseName()));

    qDebug() << "DataContainer created | " << this->objectName();
}

QMap<QString,QString>& DataContainer::defineFilesAvailability()
{
    // parsing 'mainData' keys
    QMap<QString, QString>::const_iterator i;
    for (i = mainData.constBegin(); i != mainData.constEnd(); ++i) {
        if (i.value() == "unreadable") {
            if (QFileInfo::exists(i.key()))
                filesAvailability[i.key()] = "on Disk (unreadable)";
            else {
                filesAvailability[i.key()] = "LOST file (unreadable)";
                lostFiles.append(i.key());
            }
        }

        else if (QFileInfo::exists(i.key())) {
            filesAvailability[i.key()] = "on Disk";
            onDiskFiles.append(i.key());
        }
        else {
            filesAvailability[i.key()] = "LOST file";
            lostFiles.append(i.key());
        }
    }

    // looking for new files
    QStringList actualFiles;
    Files files(workDir);

    if (!onlyExtensions.isEmpty())
        actualFiles = files.filteredFileList(onlyExtensions, true);
    else {
        QStringList ignoreList(ignoredExtensions);
        ignoreList.append({"ver.json", "sha1", "sha256", "sha512"});
        actualFiles = files.filteredFileList(ignoreList); // all files from workDir except ignored extensions and *.ver.json and *.sha1/256/512
    }

    foreach (const QString &file, actualFiles) {
        if (!mainData.contains(file)) {
            filesAvailability[file] = "NEW file";
            newFiles.append(file);
        }
    }

    return filesAvailability;
}

QMap<QString,QString> DataContainer::newlostOnly()
{
    QMap<QString,QString> newlost;

    if (lostFiles.size() > 0) {
        newlost.insert(fillMapSameValues(lostFiles, "LOST file"));
    }

    if (newFiles.size() > 0) {
        newlost.insert(fillMapSameValues(newFiles, "NEW file"));
    }

    return newlost;
}

QMap<QString,QString> DataContainer::clearDataFromLostFiles()
{
    if (lostFiles.size() > 0) {
        foreach (const QString &file, lostFiles) {
            mainData.remove(file);
        }

        QMap<QString,QString> changes = fillMapSameValues(lostFiles, "removed from DB");
        lostFiles.clear();
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
            qDebug()<< "DataContainer::updateMainData() | mainData updated, 'newFiles' cleared";
        }
        return changes;
    }
    else {
        qDebug() << "DataContainer::updateMainData() | NO Data to add";
        return QMap<QString,QString>();
    }
}

QMap<QString,QString> DataContainer::listFolderContents(const QString &rootFolder)
{
    QMap<QString, QString> content;
    QMapIterator<QString,QString> iter(filesAvailability);

    while (iter.hasNext()) {
        iter.next();
        if (iter.key().startsWith(rootFolder))
            content.insert(iter.key(), iter.value());
        else if (!content.isEmpty()) {
            break; // is Relative End; QMap is sorted by key, so there is no point in itering to the real end.
        }
    }

    return content;
}

QString DataContainer::itemContentsInfo(const QString &itemPath)
{
    QString fullPath = Files::joinPath(workDir, itemPath);
    QFileInfo fInf(fullPath);
    if (fInf.isFile())
        return Files::fileSize(fullPath);
    else if (fInf.isDir()) {
        QMap<QString, QString> content = listFolderContents(fullPath + '/');
        QMapIterator<QString,QString> iterContent(content);
        QStringList ondisk;
        QStringList lost;
        QStringList newfiles;

        while (iterContent.hasNext()) {
            iterContent.next();
            if (iterContent.value() == "on Disk") {
                ondisk.append(iterContent.key());
            }
            else if (iterContent.value() == "LOST file") {
                lost.append(iterContent.key());
            }
            else if (iterContent.value() == "NEW file") {
                newfiles.append(iterContent.key());
            }
            else
                qDebug()<< "DataContainer::itemContents | Ambiguous item: " << iterContent.key();
        }

        QString text;
        if (ondisk.size() > 0) {
            text = "on Disk: " + Files().contentStatus(ondisk);
        }

        if (lost.size() > 0) {
            QString pre;
            if (ondisk.size() > 0)
                pre = "; ";
            text.append(QString("%1Lost files: %2").arg(pre).arg(lost.size()));
        }

        if (newfiles.size() > 0) {
            QString pre;
            if (ondisk.size() > 0 || lost.size() > 0)
                pre = "; ";
            text.append(QString("%1New: %2").arg(pre, Files().contentStatus(newfiles)));
        }

        return text;
    }
    else
        return "The item actually not on a disk";
}

QString DataContainer::aboutDb()
{
    QString newFilesInfo;

    if (newFiles.size() > 0) {
        newFilesInfo = "New: " + Files().contentStatus(newFiles);
    }
    else
        newFilesInfo = "New files: 0";

    QString filters;
    if (!ignoredExtensions.isEmpty()) {
        filters = QString("\nIgnored: %1").arg(ignoredExtensions.join(", "));
    }
    else if (!onlyExtensions.isEmpty()) {
        filters = QString("\nIncluded Only: %1").arg(onlyExtensions.join(", "));
    }

    int filesAvailabilityNumber = filesAvailability.size();
    int storedPathsNumber = mainData.size();

    QString storedPathsInfo;
    if (filesAvailabilityNumber != storedPathsNumber) {
        storedPathsInfo = QString("Stored paths: %1\n").arg(storedPathsNumber);
    }

    QString tipText;

    if (newFiles.size() > 0 || lostFiles.size() > 0) {
        tipText = "\n\nUse context menu for more options";
    }

    return QString("Algorithm: SHA-%1%2\nStored size: %3\nLast update: %4\n\nTotal files listed: %5\n%6%7\nLost files: %8%9")
        .arg(shaType()).arg(filters, storedDataSize, lastUpdate).arg(filesAvailabilityNumber).arg(storedPathsInfo, newFilesInfo).arg(lostFiles.size()).arg(tipText);
}

QMap<QString,QString> DataContainer::fillMapSameValues(const QStringList &keys, const QString &value)
{
    QMap<QString,QString> resultMap;

    foreach (const QString &key, keys) {
        resultMap.insert(key, value);
    }

    return resultMap;
}

void DataContainer::setIgnoredExtensions(const QStringList &extensions)
{
    ignoredExtensions = extensions;
    onlyExtensions.clear();
}

void DataContainer::setOnlyExtensions(const QStringList &extensions)
{
    onlyExtensions = extensions;
    ignoredExtensions.clear();
}

void DataContainer::setJsonFileNamePrefix(const QString &prefix)
{
    jsonFilePath = Files::joinPath(workDir, QString("%1_%2.ver.json").arg(prefix, Files::folderName(workDir)));
}

int DataContainer::shaType()
{
    if (shatype != 0)
        return shatype;

    int len = mainData.begin().value().size();
    if (len == 40)
        return 1;
    else if (len == 64)
        return 256;
    else if (len == 128)
        return 512;
    else
        return 0;
}

DataContainer::~DataContainer()
{
    qDebug()<< "DataContainer deleted | " << this->objectName();
}
