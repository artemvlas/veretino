#include "datacontainer.h"
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
    updateNumbers();

    qDebug() << "DataMaintainer created | " << data_.metaData.workDir;
}

void DataMaintainer::updateNumbers()
{
    Numbers num;

    FileList::const_iterator iter;
    for (iter = data_.filesData.constBegin(); iter != data_.filesData.constEnd(); ++iter) {
        if (!iter.value().checksum.isEmpty())
            ++num.numChecksums;

        switch (iter.value().status) {
        case FileValues::NotChecked:
            ++num.numNotChecked;
            num.totalSize += iter.value().size;
            break;
        case FileValues::Matched:
            ++num.numMatched;
            num.totalSize += iter.value().size;
            break;
        case FileValues::Mismatched:
            ++num.numMismatched;
            num.totalSize += iter.value().size;
            break;
        case FileValues::New:
            ++num.numNewFiles;
            break;
        case FileValues::Missing:
            ++num.numMissingFiles;
            break;
        case FileValues::Unreadable:
            ++num.numUnreadable;
            break;
        case FileValues::Added:
            ++num.numMatched;
            num.totalSize += iter.value().size;
            break;
        case FileValues::Removed:
            break;
        case FileValues::ChecksumUpdated:
            ++num.numMatched;
            num.totalSize += iter.value().size;
            break;
        }
    }

    num.numAvailable = num.numNotChecked + num.numMatched + num.numMismatched;
    data_.numbers = num;

    QString newmissing;
    QString mismatched;
    QString matched;
    QString sep;

    if (num.numNewFiles > 0 || num.numMissingFiles > 0)
        newmissing = "* ";

    if (num.numMismatched > 0)
        mismatched = QString("☒%1").arg(num.numMismatched);
    if (num.numMatched > 0)
        matched = QString(" ✓%1").arg(num.numMatched);

    if (num.numMismatched > 0 || num.numMatched > 0)
        sep = " : ";

    QString checkStatus = QString("\t%1%2%3%4").arg(newmissing, mismatched, matched, sep);

    emit setPermanentStatus(QString("%1%2 avail. | %3 | %4")
                            .arg(checkStatus)
                            .arg(num.numAvailable)
                            .arg(format::dataSizeReadable(num.totalSize), format::algoToStr(data_.metaData.algorithm)));
}

void DataMaintainer::updateFilesValues()
{
    emit setStatusbarText("Defining files values...");
    canceled = false;
    QMutableMapIterator<QString, FileValues> iter(data_.filesData);

    while (iter.hasNext() && !canceled) {
        QString fullPath = paths::joinPath(data_.metaData.workDir, iter.next().key());
        if (QFileInfo::exists(fullPath))
            iter.value().size = QFileInfo(fullPath).size();
        else
            iter.value().status = FileValues::Missing;
    }

    if (canceled) {
        qDebug() << "DataMaintainer::updateFilesValues() | Canceled:" << data_.metaData.workDir;
    }
}

int DataMaintainer::findNewFiles()
{
    emit setStatusbarText("Looking for new files...");

    canceled = false;
    int number = 0;
    QDir dir(data_.metaData.workDir);
    QDirIterator it(data_.metaData.workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !canceled) {
        QString fullPath = it.next();
        QString relPath = dir.relativeFilePath(fullPath);

        if (paths::isFileAllowed(fullPath, data_.metaData.filter) && !data_.filesData.contains(relPath)) {
            QFileInfo fileInfo(fullPath);
            // unreadable files are ignored
            if (fileInfo.isReadable()) {
                FileValues curFileValues;
                curFileValues.status = FileValues::New;
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

FileList DataMaintainer::listOf(FileValues::FileStatus fileStatus)
{
    return listOf(fileStatus, data_.filesData);
}

FileList DataMaintainer::listOf(FileValues::FileStatus fileStatus, const FileList &originalList)
{
    FileList resultList;
    FileList::const_iterator iter;

    for (iter = originalList.constBegin(); iter != originalList.constEnd(); ++iter) {
        if (iter.value().status == fileStatus)
            resultList.insert(iter.key(), iter.value());
    }

    return resultList;
}

FileList DataMaintainer::listOf(QSet<FileValues::FileStatus> fileStatuses)
{
    FileList resultList;
    FileList::const_iterator iter;

    for (iter = data_.filesData.constBegin(); iter != data_.filesData.constEnd(); ++iter) {
        if (fileStatuses.contains(iter.value().status))
            resultList.insert(iter.key(), iter.value());
    }

    return resultList;
}

int DataMaintainer::clearDataFromLostFiles()
{
    int number = 0;
    QMutableMapIterator<QString, FileValues> iter(data_.filesData);

    while (iter.hasNext()) {
        iter.next();
        if (iter.value().status == FileValues::Missing) {
            iter.value().checksum.clear();
            iter.value().status = FileValues::Removed;
            model_->setItemStatus(iter.key(), FileValues::Removed);
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
            model_->setItemStatus(iter.key(), FileValues::ChecksumUpdated);
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
        model_->setItemStatus(iter.key(), iter.value().status);
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
    model_->setItemStatus(filePath, curFileValues.status);

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
    updateNumbers();
    data_.metaData.saveDateTime = format::currentDateTime();

    JsonDb *json = new JsonDb;
    connect(json, &JsonDb::setStatusbarText, this, &DataMaintainer::setStatusbarText);
    connect(json, &JsonDb::showMessage, this, &DataMaintainer::showMessage);

    json->makeJson(data_);

    json->deleteLater();
}

FileList DataMaintainer::subfolderContent(QString subFolder)
{
    if (subFolder.isEmpty())
        return data_.filesData;

    if (!subFolder.endsWith('/'))
        subFolder.append('/');

    FileList content;
    FileList::const_iterator iter;

    for (iter = data_.filesData.constBegin(); iter != data_.filesData.constEnd(); ++iter) {
        if (iter.key().startsWith(subFolder))
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
        FileList content = subfolderContent(itemPath);
        FileList newfiles;
        FileList ondisk;
        int lost = 0;

        FileList::const_iterator iterContent;
        for (iterContent = content.constBegin(); iterContent != content.constEnd(); ++iterContent) {
            if (iterContent.value().status == FileValues::New)
                newfiles.insert(iterContent.key(), iterContent.value());
            else if (iterContent.value().status == FileValues::Missing)
                ++lost;
            else if (iterContent.value().status != FileValues::Unreadable)
                ondisk.insert(iterContent.key(), iterContent.value());
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
    updateNumbers();

    QString result("DB filename: ");
    if (data_.metaData.databaseFileName.contains('/')) {
        result.append(QFileInfo(data_.metaData.databaseFileName).fileName());
        result.append("\nWorkDir: " + data_.metaData.workDir);
    }
    else
        result.append(data_.metaData.databaseFileName);

    result.append(QString("\nAlgorithm: %1").arg(format::algoToStr(data_.metaData.algorithm)));

    if (!data_.metaData.filter.extensionsList.isEmpty()) {
        if (data_.metaData.filter.includeOnly)
            result.append(QString("\nIncluded Only: %1").arg(data_.metaData.filter.extensionsList.join(", ")));
        else
            result.append(QString("\nIgnored: %1").arg(data_.metaData.filter.extensionsList.join(", ")));
    }

    result.append(QString("\nLast update: %1").arg(data_.metaData.saveDateTime));

    if (data_.numbers.numChecksums != data_.numbers.numAvailable)
        result.append(QString("\n\nStored checksums: %1").arg(data_.numbers.numChecksums));
    else
        result.append("\n");

    if (data_.numbers.numAvailable > 0)
        result.append(QString("\nAvailable: %1").arg(format::filesNumberAndSize(data_.numbers.numAvailable, data_.numbers.totalSize)));
    else
        result.append("\nNO FILES available to check");

    if (data_.numbers.numUnreadable > 0)
        result.append(QString("\nUnreadable files: %1").arg(data_.numbers.numUnreadable));

    if (data_.numbers.numNewFiles > 0)
        result.append("\n\nNew: " + Files::contentStatus(listOf(FileValues::New)));
    else
        result.append("\n\nNo New files found");

    if (data_.numbers.numMissingFiles > 0)
        result.append(QString("\nMissing: %1 files").arg(data_.numbers.numMissingFiles));
    else
        result.append("\nNo Missing files found");

    if (data_.numbers.numAvailable > 0) {
        if (data_.numbers.numNotChecked == 0) {
            if (data_.numbers.numMismatched > 0)
                result.append(QString("\n\n☒ %1 mismatches of %2 checksums").arg(data_.numbers.numMismatched).arg(data_.numbers.numChecksums));
            else if (data_.numbers.numChecksums == data_.numbers.numAvailable)
                result.append(QString("\n\n✓ ALL %1 stored checksums matched").arg(data_.numbers.numChecksums));
            else
                result.append(QString("\n\n✓ All %1 available files matched the stored checksums").arg(data_.numbers.numAvailable));
        }
        else if (data_.numbers.numNewFiles > 0 || data_.numbers.numMissingFiles > 0)
            result.append("\n\nUse context menu for more options");
    }

    emit showMessage(result, "Database status");
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
