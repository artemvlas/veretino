#include "datacontainer.h"
#include "files.h"
#include "tools.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include "jsondb.h"
#include <QDirIterator>

DataMaintainer::DataMaintainer(QObject *parent)
    : QObject(parent)
{}

DataMaintainer::DataMaintainer(const DataContainer &initData, QObject *parent)
    : QObject(parent), data_(initData)
{
    updateMetaData();

    qDebug() << "DataMaintainer created | " << data_.metaData.workDir;
}

void DataMaintainer::updateMetaData()
{
    if (data_.metaData.shaType != 1 && data_.metaData.shaType != 256 && data_.metaData.shaType != 512)
        data_.metaData.shaType = shaType(data_.filesData);

    data_.metaData.numNewFiles = 0;
    data_.metaData.numMissingFiles = 0;
    data_.metaData.numChecksums = 0;
    data_.metaData.numAvailable = 0;
    data_.metaData.totalSize = 0;

    FileList::const_iterator iter;
    for (iter = data_.filesData.constBegin(); iter != data_.filesData.constEnd(); ++iter) {
        if (!iter.value().checksum.isEmpty()) {           
            ++data_.metaData.numChecksums;
            if (iter.value().exists) {
                data_.metaData.totalSize += iter.value().size;
                ++data_.metaData.numAvailable;
            }
            else
                ++data_.metaData.numMissingFiles;
        }
        if (iter.value().isNew)
            ++data_.metaData.numNewFiles;
    }

    emit setPermanentStatus(QString("SHA-%1 | %2 avail. | %3")
                            .arg(data_.metaData.shaType)
                            .arg(data_.metaData.numAvailable)
                            .arg(format::dataSizeReadable(data_.metaData.totalSize)));
}

void DataMaintainer::updateFilesValues()
{
    emit status("Defining files values...");
    canceled = false;
    QMutableMapIterator<QString, FileValues> iter(data_.filesData);

    while (iter.hasNext() && !canceled) {
        QString fullPath = paths::joinPath(data_.metaData.workDir, iter.next().key());
        iter.value().exists = QFileInfo::exists(fullPath);
        if (iter.value().exists && iter.value().isReadable) {
            iter.value().size = QFileInfo(fullPath).size();
        }
    }

    if (canceled) {
        qDebug() << "DataMaintainer::updateFilesValues() | Canceled:" << data_.metaData.workDir;
    }
}

int DataMaintainer::findNewFiles()
{
    emit status("Looking for new files...");

    FilterRule filter = data_.metaData.filter;
    if (filter.extensionsList.isEmpty() || !filter.include) {
        filter.include = false;
        filter.extensionsList.append({"ver.json", "sha1", "sha256", "sha512"}); // <-- ignore these types while looking for new files
    }

    canceled = false;
    int number = 0;
    QDir dir(data_.metaData.workDir);
    QDirIterator it(data_.metaData.workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !canceled) {
        QString fullPath = it.next();
        QString relPath = dir.relativeFilePath(fullPath);

        if (paths::isFileAllowed(fullPath, filter) && !data_.filesData.contains(relPath)) {
            QFileInfo fileInfo(fullPath);
            // unreadable files are ignored
            if (fileInfo.isReadable()) {
                FileValues curFileValues;
                curFileValues.isNew = true;
                curFileValues.size = fileInfo.size();
                data_.filesData.insert(relPath, curFileValues);
                ++number;
            }
        }
    }

    if (canceled) {
        number = 0;
        qDebug() << "DataMaintainer::findNewFiles() | Canceled:" << data_.metaData.workDir;
    }

    return number;
}

DataContainer DataMaintainer::availableFiles()
{
    DataContainer curData(data_.metaData);
    FileList::const_iterator iter;

    for (iter = data_.filesData.constBegin(); iter != data_.filesData.constEnd(); ++iter) {
        if (!iter.value().isNew && iter.value().exists && iter.value().isReadable) {
            curData.filesData.insert(iter.key(), iter.value());
        }
    }

    return curData;
}

DataContainer DataMaintainer::newFiles()
{
    DataContainer curData(data_.metaData);
    FileList::const_iterator iter;

    for (iter = data_.filesData.constBegin(); iter != data_.filesData.constEnd(); ++iter) {
        if (iter.value().isNew) {
            curData.filesData.insert(iter.key(), iter.value());
        }
    }

    return curData;
}

FileList DataMaintainer::newlostOnly()
{
    FileList newlost;
    FileList::const_iterator iter;

    for (iter = data_.filesData.constBegin(); iter != data_.filesData.constEnd(); ++iter) {
        if (iter.value().isNew || !iter.value().exists) {
            newlost.insert(iter.key(), iter.value());
        }
    }

    return newlost;
}

FileList DataMaintainer::changesOnly()
{
    FileList resultMap;
    FileList::const_iterator iter;

    for (iter = data_.filesData.constBegin(); iter != data_.filesData.constEnd(); ++iter) {
        if (!iter.value().about.isEmpty()) {
            resultMap.insert(iter.key(), iter.value());
        }
    }

    return resultMap;
}

int DataMaintainer::clearDataFromLostFiles()
{
    int number = 0;
    QMutableMapIterator<QString, FileValues> iter(data_.filesData);

    while (iter.hasNext()) {
        iter.next();
        if (!iter.value().exists) {
            iter.value().checksum.clear();
            iter.value().about = "removed from DB";
            ++number;
        }
    }
    return number;
}

int DataMaintainer::updateMismatchedChecksums()
{
    int number = 0;
    QMutableMapIterator<QString, FileValues> iter(data_.filesData);

    while (iter.hasNext()) {
        iter.next();
        if (!iter.value().reChecksum.isEmpty()) {
            iter.value().checksum = iter.value().reChecksum;
            iter.value().reChecksum.clear();
            iter.value().about = "stored checksum updated";
            ++number;
        }
    }
    return number;
}

void DataMaintainer::updateData(const FileList &updateFiles)
{
    data_.filesData.insert(updateFiles);
}

void DataMaintainer::importJson(const QString &jsonFilePath)
{
    JsonDb *json = new JsonDb;
    connect(json, &JsonDb::status, this, &DataMaintainer::status);
    connect(json, &JsonDb::showMessage, this, &DataMaintainer::showMessage);

    data_ = json->parseJson(jsonFilePath);
    json->deleteLater();

    if (data_.filesData.isEmpty()) {
        return;
    }

    if (!canceled)
        updateFilesValues();
    if (!canceled)
        findNewFiles();
    else
        data_.filesData.clear();
}

void DataMaintainer::exportToJson()
{
    JsonDb *json = new JsonDb;
    connect(json, &JsonDb::status, this, &DataMaintainer::status);
    connect(json, &JsonDb::showMessage, this, &DataMaintainer::showMessage);

    json->makeJson(data_);

    json->deleteLater();
}

FileList DataMaintainer::listFolderContents(QString rootFolder)
{
    if (!rootFolder.endsWith('/'))
        rootFolder.append('/');

    FileList content;
    FileList::const_iterator iter;

    for (iter = data_.filesData.constBegin(); iter != data_.filesData.constEnd(); ++iter) {
        if (iter.key().startsWith(rootFolder))
            content.insert(iter.key(), iter.value());
        else if (!content.isEmpty()) {
            break; // is Relative End; QMap is sorted by key, so there is no point in itering to the real end.
        }
    }

    return content;
}

QString DataMaintainer::itemContentsInfo(const QString &itemPath)
{
    QString fullPath = paths::joinPath(data_.metaData.workDir, itemPath);
    QFileInfo fileInfo(fullPath);
    if (fileInfo.isFile())
        return format::fileNameAndSize(fullPath);
    else if (fileInfo.isDir()) {
        FileList content = listFolderContents(itemPath);        
        FileList newfiles;
        FileList ondisk;
        int lost = 0;

        FileList::const_iterator iterContent;
        for (iterContent = content.constBegin(); iterContent != content.constEnd(); ++iterContent) {
            if (iterContent.value().isNew) {
                newfiles.insert(iterContent.key(), iterContent.value());
            }
            else if (iterContent.value().exists && iterContent.value().isReadable) {
                ondisk.insert(iterContent.key(), iterContent.value());
            }
            else if (!iterContent.value().exists) {
                ++lost;
            }
            else
                qDebug()<< "DataMaintainer::itemContents | Ambiguous item: " << iterContent.key();
        }

        QString text;
        if (ondisk.size() > 0) {
            text = "on Disk: " + Files::contentStatus(ondisk);
        }

        if (lost > 0) {
            QString pre;
            if (ondisk.size() > 0)
                pre = "; ";
            text.append(QString("%1Lost files: %2").arg(pre).arg(lost));
        }

        if (newfiles.size() > 0) {
            QString pre;
            if (ondisk.size() > 0 || lost > 0)
                pre = "; ";
            text.append(QString("%1New: %2").arg(pre, Files::contentStatus(newfiles)));
        }

        return text;
    }
    else
        return "The item actually not on a disk";
}

void DataMaintainer::aboutDb()
{
    updateMetaData();

    QString result("DB filename: " + data_.metaData.databaseFileName);
    result.append(QString("\nAlgorithm: SHA-%1").arg(data_.metaData.shaType));

    if (!data_.metaData.filter.extensionsList.isEmpty()) {
        if (data_.metaData.filter.include)
            result.append(QString("\nIncluded Only: %1").arg(data_.metaData.filter.extensionsList.join(", ")));
        else
            result.append(QString("\nIgnored: %1").arg(data_.metaData.filter.extensionsList.join(", ")));
    }

    result.append(QString("\nLast update: %1").arg(data_.metaData.saveDateTime));

    if (data_.metaData.numAvailable != data_.filesData.size())
        result.append(QString("\n\nTotal files listed: %1").arg(data_.filesData.size()));

    if (data_.metaData.numChecksums != data_.filesData.size())
        result.append(QString("\nStored checksums: %1").arg(data_.metaData.numChecksums));

    result.append(QString("\n\nAvailable: %1").arg(format::filesNumberAndSize(data_.metaData.numAvailable, data_.metaData.totalSize)));

    if (data_.metaData.numNewFiles > 0) {
        result.append("\n\nNew: " + Files::contentStatus(newFiles().filesData));
    }
    else
        result.append("\n\nNo New files found");

    if (data_.metaData.numMissingFiles > 0) {
        result.append(QString("\nMissing: %1 files").arg(data_.metaData.numMissingFiles));
    }
    else
        result.append("\nNo Missing files found");

    if (data_.metaData.numNewFiles > 0 || data_.metaData.numMissingFiles > 0) {
        result.append("\n\nUse context menu for more options");
    }

    emit showMessage(result, "Database status");
}

int DataMaintainer::shaType(const FileList &fileList)
{
    int len = 0;

    FileList::const_iterator iter;
    for (iter = fileList.constBegin(); len == 0 && iter != fileList.constEnd(); ++iter)  {
        if (!iter.value().checksum.isEmpty())
            len = iter.value().checksum.size();
    }

    if (len == 40) {
        return 1;
    }
    else if (len == 64) {
        return 256;
    }
    else if (len == 128) {
        return 512;
    }
    else {
        qDebug() << "DataMaintainer::shaType() | Failed to determine the Algorithm. Invalid string length:" << len;
        return 0;
    }
}

void DataMaintainer::cancelProcess()
{
    canceled = true;
}

DataMaintainer::~DataMaintainer()
{
    qDebug()<< "DataMaintainer deleted | " << data_.metaData.workDir;
    emit setPermanentStatus();
}
