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
    data_.metaData.numMatched = 0;
    data_.metaData.numMismatched = 0;
    data_.metaData.numAvailable = 0;
    data_.metaData.numUnreadable = 0;
    data_.metaData.totalSize = 0;

    FileList::const_iterator iter;
    for (iter = data_.filesData.constBegin(); iter != data_.filesData.constEnd(); ++iter) {
        if (iter.value().isNew)
            ++data_.metaData.numNewFiles;
        else if (!iter.value().isReadable)
            ++data_.metaData.numUnreadable;
        else if (!iter.value().checksum.isEmpty()) {
            ++data_.metaData.numChecksums;
            if (!iter.value().reChecksum.isEmpty())
                ++data_.metaData.numMismatched;
            if (iter.value().exists) {
                data_.metaData.totalSize += iter.value().size;
                ++data_.metaData.numAvailable;
            }
            else
                ++data_.metaData.numMissingFiles;
        }
        if (iter.value().status == FileValues::Matched
            || iter.value().status == FileValues::Added
            || iter.value().status == FileValues::ChecksumUpdated)
            ++data_.metaData.numMatched;
    }

    QString checkStatus("\t");

    if (data_.metaData.numMismatched > 0)
        checkStatus.append(QString("☒%1").arg(data_.metaData.numMismatched));
    if (data_.metaData.numMatched > 0)
        checkStatus.append(QString(" ✓%1").arg(data_.metaData.numMatched));

    if (checkStatus.size() > 2)
        checkStatus.append(" : ");

    emit setPermanentStatus(QString("%1%2 avail. | %3 | SHA-%4")
                            .arg(checkStatus)
                            .arg(data_.metaData.numAvailable)
                            .arg(format::dataSizeReadable(data_.metaData.totalSize))
                            .arg(data_.metaData.shaType));
}

void DataMaintainer::updateFilesValues()
{
    emit setStatusbarText("Defining files values...");
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
    emit setStatusbarText("Looking for new files...");

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

FileList DataMaintainer::listOf(Listing only)
{
    FileList result;
    FileList::const_iterator iter;

    for (iter = data_.filesData.constBegin(); iter != data_.filesData.constEnd(); ++iter) {
        switch (only) {
            case Available:
                if (!iter.value().isNew && iter.value().exists && iter.value().isReadable)
                    result.insert(iter.key(), iter.value());
                break;
            case New:
                if (iter.value().isNew)
                    result.insert(iter.key(), iter.value());
                break;
            case Lost:
                if (!iter.value().exists)
                    result.insert(iter.key(), iter.value());
                break;
            /*case Changes:
                if (iter.value().status != 0)
                    result.insert(iter.key(), iter.value());
                break;*/
            case Mismatches:
                if (!iter.value().reChecksum.isEmpty())
                    result.insert(iter.key(), iter.value());
                break;
            case Added:
                if (iter.value().status == FileValues::Added)
                    result.insert(iter.key(), iter.value());
                break;
            case Removed:
                if (iter.value().status == FileValues::Removed)
                    result.insert(iter.key(), iter.value());
                break;
            case Updated:
                if (iter.value().status == FileValues::ChecksumUpdated)
                    result.insert(iter.key(), iter.value());
                break;
        }
    }

    return result;
}

FileList DataMaintainer::listOf(QList<Listing> only)
{
    FileList resultList;
    for (int i = 0; i < only.size(); ++i) {
        resultList.insert(listOf(only.at(i)));
    }
    return resultList;
}

int DataMaintainer::clearDataFromLostFiles()
{
    int number = 0;
    QMutableMapIterator<QString, FileValues> iter(data_.filesData);

    while (iter.hasNext()) {
        iter.next();
        if (!iter.value().exists) {
            iter.value().checksum.clear();
            iter.value().status = FileValues::Removed;
            emit itemStatusChanged(iter.key(), FileValues::Removed);
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
            iter.value().status = FileValues::ChecksumUpdated;
            emit itemStatusChanged(iter.key(), FileValues::ChecksumUpdated);
            ++number;
        }
    }

    return number;
}

int DataMaintainer::updateData(const FileList &updateFiles)
{
    if (!updateFiles.isEmpty())
        data_.filesData.insert(updateFiles);

    return updateFiles.size();
}

int DataMaintainer::updateData(FileList updateFiles, FileValues::FileStatus status)
{
    QMutableMapIterator<QString, FileValues> iter(updateFiles);

    while (iter.hasNext()) {
        iter.next();
        iter.value().status = status;
        emit itemStatusChanged(iter.key(), iter.value().status);
    }

    return updateData(updateFiles);
}

bool DataMaintainer::updateData(const QString &filePath, const QString &checksum)
{
    FileValues curFileValues = data_.filesData.value(filePath);

    if (checksum == curFileValues.checksum) {
        curFileValues.status = FileValues::Matched;
    }
    else {
        curFileValues.reChecksum = checksum;
        curFileValues.status = FileValues::Mismatched;
    }

    data_.filesData.insert(filePath, curFileValues);
    emit itemStatusChanged(filePath, curFileValues.status);

    return (curFileValues.status == FileValues::Matched);
}

void DataMaintainer::importJson(const QString &jsonFilePath)
{
    JsonDb *json = new JsonDb;
    connect(json, &JsonDb::setStatusbarText, this, &DataMaintainer::setStatusbarText);
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
    updateMetaData();

    JsonDb *json = new JsonDb;
    connect(json, &JsonDb::setStatusbarText, this, &DataMaintainer::setStatusbarText);
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
                qDebug() << "DataMaintainer::itemContents | Ambiguous item: " << iterContent.key();
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

void DataMaintainer::dbStatus()
{
    updateMetaData();

    QString result("DB filename: ");
    if (data_.metaData.databaseFileName.contains('/')) {
        result.append(QFileInfo(data_.metaData.databaseFileName).fileName());
        result.append("\nWorkDir: " + data_.metaData.workDir);
    }
    else
        result.append(data_.metaData.databaseFileName);

    result.append(QString("\nAlgorithm: SHA-%1").arg(data_.metaData.shaType));

    if (!data_.metaData.filter.extensionsList.isEmpty()) {
        if (data_.metaData.filter.include)
            result.append(QString("\nIncluded Only: %1").arg(data_.metaData.filter.extensionsList.join(", ")));
        else
            result.append(QString("\nIgnored: %1").arg(data_.metaData.filter.extensionsList.join(", ")));
    }

    result.append(QString("\nLast update: %1").arg(data_.metaData.saveDateTime));

    if (data_.metaData.numChecksums != data_.metaData.numAvailable)
        result.append(QString("\n\nStored checksums: %1").arg(data_.metaData.numChecksums));
    else
        result.append("\n");

    result.append(QString("\nAvailable: %1").arg(format::filesNumberAndSize(data_.metaData.numAvailable, data_.metaData.totalSize)));

    if (data_.metaData.numUnreadable > 0)
        result.append(QString("\nUnreadable files: %1").arg(data_.metaData.numUnreadable));

    if (data_.metaData.numNewFiles > 0)
        result.append("\n\nNew: " + Files::contentStatus(listOf(Listing::New)));
    else
        result.append("\n\nNo New files found");

    if (data_.metaData.numMissingFiles > 0)
        result.append(QString("\nMissing: %1 files").arg(data_.metaData.numMissingFiles));
    else
        result.append("\nNo Missing files found");

    if (data_.metaData.isChecked) {
        if (data_.metaData.numMismatched > 0)
            result.append(QString("\n\n☒ %1 mismatches of %2 checksums").arg(data_.metaData.numMismatched).arg(data_.metaData.numChecksums));
        else if (data_.metaData.numChecksums == data_.metaData.numAvailable)
            result.append(QString("\n\n✓ ALL %1 stored checksums matched").arg(data_.metaData.numChecksums));
        else
            result.append(QString("\n\n✓ All %1 available files matched the stored checksums").arg(data_.metaData.numAvailable));
    }
    else if (data_.metaData.numNewFiles > 0 || data_.metaData.numMissingFiles > 0) {
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
