// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "datacontainer.h"
#include "tools.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include "jsondb.h"
#include <QDirIterator>
#include "treemodeliterator.h"

DataMaintainer::DataMaintainer(QObject *parent)
    : QObject(parent)
{
    setSourceData();
}

DataMaintainer::DataMaintainer(DataContainer* initData, QObject *parent)
    : QObject(parent)
{
    setSourceData(initData);
}

void DataMaintainer::setSourceData()
{
    setSourceData(new DataContainer(this));
}

void DataMaintainer::setSourceData(DataContainer *sourceData)
{
    if (sourceData) {
        clearData();
        data_ = sourceData;
        updateNumbers();
    }
}

void DataMaintainer::clearData()
{
    if (data_) {
        delete data_;
        data_ = nullptr;
    }
}

// add new files to data_->model_
int DataMaintainer::addActualFiles(FileStatus addedFileStatus, bool ignoreUnreadable)
{
    if (!data_)
        return 0;

    if (!QFileInfo(data_->metaData.workDir).isDir()) {
        qDebug() << "DataMaintainer::addActualFiles | Not a folder path: " << data_->metaData.workDir;
        return 0;
    }

    emit setStatusbarText("Creating a list of files...");

    canceled = false;
    int numAdded = 0;

    QDir dir(data_->metaData.workDir);
    QDirIterator it(data_->metaData.workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !canceled) {
        QString fullPath = it.next();
        QString relPath = dir.relativeFilePath(fullPath);

        if (paths::isFileAllowed(relPath, data_->metaData.filter)) {
            FileValues curFileValues;
            QFileInfo fileInfo(fullPath);
            if (fileInfo.isReadable()) {
                curFileValues.status = addedFileStatus;
                curFileValues.size = fileInfo.size(); // If the file is unreadable, then its size is not needed
            }
            else if (!ignoreUnreadable)
                curFileValues.status = FileStatus::Unreadable;
            else
                continue;

            if (data_->model_->addFile(relPath, curFileValues))
                ++numAdded;
        }
    }

    if (canceled) {
        qDebug() << "DataMaintainer::addActualFiles | Canceled:" << data_->metaData.workDir;
        clearData();
        emit setStatusbarText();
        return 0;
    }

    //emit setStatusbarText(QString("%1 files found").arg(numAdded));
    updateNumbers();

    return numAdded;
}

void DataMaintainer::updateNumbers()
{
    if (!data_) {
        qDebug() << "DataMaintainer::updateNumbers | NO data_";
        return;
    }

    data_->numbers = updateNumbers(data_->model_);

    QString newmissing;
    QString mismatched;
    QString matched;
    QString sep;

    if (data_->numbers.numNewFiles > 0 || data_->numbers.numMissingFiles > 0)
        newmissing = "* ";

    if (data_->numbers.numMismatched > 0)
        mismatched = QString("☒%1").arg(data_->numbers.numMismatched);
    if (data_->numbers.numMatched > 0)
        matched = QString(" ✓%1").arg(data_->numbers.numMatched + data_->numbers.numAdded + data_->numbers.numChecksumUpdated);

    if (data_->numbers.numMismatched > 0 || data_->numbers.numMatched > 0)
        sep = " : ";

    QString checkStatus = QString("\t%1%2%3%4").arg(newmissing, mismatched, matched, sep);

    emit setPermanentStatus(QString("%1%2 avail. | %3 | %4")
                            .arg(checkStatus)
                            .arg(data_->numbers.numAvailable)
                            .arg(format::dataSizeReadable(data_->numbers.totalSize), format::algoToStr(data_->metaData.algorithm)));
}

Numbers DataMaintainer::updateNumbers(const QAbstractItemModel *model, const QModelIndex &rootIndex)
{
    Numbers num;

    TreeModelIterator iter(model, rootIndex);

    while (iter.hasNext()) {
        iter.nextFile();

        if (iter.data(ModelKit::ColumnChecksum).isValid() && !iter.data(ModelKit::ColumnChecksum).toString().isEmpty()) {
            ++num.numChecksums;
            num.totalSize += iter.data(ModelKit::ColumnSize).toLongLong();
        }

        if (!iter.data(ModelKit::ColumnStatus).isValid())
            continue;

        switch (iter.status()) {
            case FileStatus::Queued:
                ++num.numQueued;
                break;
            case FileStatus::NotChecked:
                ++num.numNotChecked;
                break;
            case FileStatus::Matched:
                ++num.numMatched;
                break;
            case FileStatus::Mismatched:
                ++num.numMismatched;
                break;
            case FileStatus::New:
                ++num.numNewFiles;
                break;
            case FileStatus::Missing:
                ++num.numMissingFiles;
                break;
            case FileStatus::Unreadable:
                ++num.numUnreadable;
                break;
            case FileStatus::Added:
                ++num.numAdded;
                break;
            case FileStatus::Removed:
                break;
            case FileStatus::ChecksumUpdated:
                ++num.numChecksumUpdated;
                break;
            default: break;
        }
    }

    num.numAvailable = num.numChecksums - num.numMissingFiles; //num.numNotChecked + num.numMatched + num.numMismatched;

    return num;
}

qint64 DataMaintainer::totalSizeOfListedFiles(FileStatus fileStatus, const QModelIndex &rootIndex)
{
    if (!data_) {
        qDebug() << "DataMaintainer::totalSizeOfListedFiles | NO data_";
        return 0;
    }

    return totalSizeOfListedFiles(data_->model_, {fileStatus}, rootIndex);
}

qint64 DataMaintainer::totalSizeOfListedFiles(const QSet<FileStatus> &fileStatuses, const QModelIndex &rootIndex)
{
    if (!data_) {
        qDebug() << "DataMaintainer::totalSizeOfListedFiles | NO data_";
        return 0;
    }

    return totalSizeOfListedFiles(data_->model_, fileStatuses, rootIndex);
}

qint64 DataMaintainer::totalSizeOfListedFiles(const QAbstractItemModel *model, const QSet<FileStatus> &fileStatuses, const QModelIndex &rootIndex)
{
    qint64 result = 0;
    TreeModelIterator it(model, rootIndex);

    while (it.hasNext()) {
        QVariant itData = it.nextFile().data(ModelKit::ColumnStatus);

        if (itData.isValid()
            && (fileStatuses.isEmpty()
                || fileStatuses.contains(static_cast<FileStatus>(itData.toInt())))) {

            result += it.data(ModelKit::ColumnSize).toLongLong();
        }
    }

    return result;
}

bool DataMaintainer::updateChecksum(QModelIndex fileRowIndex, const QString &computedChecksum)
{
    if (!data_ || !fileRowIndex.isValid()) {
        qDebug() << "DataMaintainer::updateChecksum | NO data_ or invalid index";
        return false;
    }

    if (fileRowIndex.model() == data_->proxyModel_)
        fileRowIndex = data_->proxyModel_->mapToSource(fileRowIndex);

    QString storedChecksum = siblingAtRow(fileRowIndex, ModelKit::ColumnChecksum).data().toString();

    if (storedChecksum.isEmpty()) {
        data_->model_->setItemData(fileRowIndex, ModelKit::ColumnChecksum, computedChecksum);
        data_->model_->setItemData(fileRowIndex, ModelKit::ColumnStatus, FileStatus::Added);
    }
    else if (storedChecksum == computedChecksum)
        data_->model_->setItemData(fileRowIndex, ModelKit::ColumnStatus, FileStatus::Matched);
    else {
        data_->model_->setItemData(fileRowIndex, ModelKit::ColumnReChecksum, computedChecksum);
        data_->model_->setItemData(fileRowIndex, ModelKit::ColumnStatus, FileStatus::Mismatched);
    }

    return (storedChecksum.isEmpty() || storedChecksum == computedChecksum);
}

int DataMaintainer::changeFilesStatuses(FileStatus currentStatus, FileStatus newStatus, const QModelIndex &rootIndex)
{
    if (!data_) {
        qDebug() << "DataMaintainer::changeFilesStatuses | NO data_";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(data_->model_, rootIndex);

    while (iter.hasNext()) {
        if (iter.nextFile().status() == currentStatus) {
            data_->model_->setItemData(iter.index(), ModelKit::ColumnStatus, newStatus);
            ++number;
        }
    }

    if (number > 0)
        updateNumbers();

    return number;
}

QString DataMaintainer::getStoredChecksum(const QModelIndex &fileRowIndex)
{
    if (!data_) {
        qDebug() << "DataMaintainer::getStoredChecksum | NO data_";
        return QString();
    }

    if (!ModelKit::isFileRow(fileRowIndex)) {
        qDebug() << "DataMaintainer::getStoredChecksum | Specified index does not belong to the file row";
        return QString();
    }

    FileStatus fileStatus = ModelKit::siblingAtRow(fileRowIndex, ModelKit::ColumnStatus)
                                        .data(ModelKit::RawDataRole).value<FileStatus>();

    QString savedSum;

    if (fileStatus == FileStatus::New)
        emit showMessage("The checksum is not yet in the database.\nPlease Update New/Lost", "NEW File");
    else if (fileStatus == FileStatus::Unreadable)
        emit showMessage("This file has been excluded (Unreadable).\nNo checksum in the database.", "Excluded File");
    else {
        savedSum = ModelKit::siblingAtRow(fileRowIndex, ModelKit::ColumnChecksum).data().toString();
        if (savedSum.isEmpty())
            emit showMessage("No checksum in the database.", "No checksum");
    }

    return savedSum;
}

int DataMaintainer::clearDataFromLostFiles()
{
    if (!data_) {
        qDebug() << "DataMaintainer::clearDataFromLostFiles | NO data_";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(data_->model_);

    while (iter.hasNext()) {
        if (iter.nextFile().status() == FileStatus::Missing) {
            data_->model_->setItemData(iter.index(), ModelKit::ColumnChecksum);
            data_->model_->setItemData(iter.index(), ModelKit::ColumnStatus, FileStatus::Removed);
            ++number;
        }
    }

    if (number > 0)
        updateNumbers();

    return number;
}

int DataMaintainer::updateMismatchedChecksums()
{
    if (!data_) {
        qDebug() << "DataMaintainer::updateMismatchedChecksums | NO data_";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(data_->model_);

    while (iter.hasNext()) {
        if (iter.nextFile().status() == FileStatus::Mismatched) {
            QString reChecksum = iter.data(ModelKit::ColumnReChecksum).toString();

            if (!reChecksum.isEmpty()) {
                data_->model_->setItemData(iter.index(), ModelKit::ColumnChecksum, reChecksum);
                data_->model_->setItemData(iter.index(), ModelKit::ColumnReChecksum);
                data_->model_->setItemData(iter.index(), ModelKit::ColumnStatus, FileStatus::ChecksumUpdated);
                ++number;
            }
        }
    }

    if (number > 0)
        updateNumbers();

    return number;
}

void DataMaintainer::importJson(const QString &jsonFilePath)
{
    JsonDb *json = new JsonDb;
    connect(json, &JsonDb::setStatusbarText, this, &DataMaintainer::setStatusbarText);
    connect(json, &JsonDb::showMessage, this, &DataMaintainer::showMessage);

    setSourceData(json->parseJson(jsonFilePath));

    json->deleteLater();

    if (canceled || data_->model_->isEmpty()) {
        clearData();
    }   
}

void DataMaintainer::exportToJson()
{
    if (!data_)
        return;

    updateNumbers();
    makeBackup();

    data_->metaData.saveDateTime = format::currentDateTime();

    JsonDb *json = new JsonDb;
    connect(json, &JsonDb::setStatusbarText, this, &DataMaintainer::setStatusbarText);
    connect(json, &JsonDb::showMessage, this, &DataMaintainer::showMessage);

    json->makeJson(data_);

    json->deleteLater();
}

QString DataMaintainer::itemContentsInfo(const QModelIndex &curIndex)
{
    QString text;

    if (ModelKit::isFileRow(curIndex)) {
        QString result;
        QVariant itemData = ModelKit::siblingAtRow(curIndex, ModelKit::ColumnSize).data(ModelKit::RawDataRole);
        if (itemData.isValid()) {
            result = QString("%1 (%2)").arg(ModelKit::siblingAtRow(curIndex, ModelKit::ColumnPath).data().toString(),
                                                                    format::dataSizeReadable(itemData.toLongLong()));
        }
        return result;
    }
    // if curIndex is at folder row
    else if (curIndex.isValid()) {
        Numbers num = updateNumbers(curIndex.model(), curIndex);
        qint64 newFilesDataSize = totalSizeOfListedFiles({FileStatus::New}, curIndex);

        if (num.numAvailable > 0) {
            text = QString("Avail.: %1")
                       .arg(format::filesNumberAndSize(num.numAvailable, num.totalSize - newFilesDataSize));
        }

        if (num.numMissingFiles > 0) {
            QString pre;
            if (num.numAvailable > 0)
                pre = "; ";
            text.append(QString("%1Missing: %2").arg(pre).arg(num.numMissingFiles));
        }

        if (num.numNewFiles > 0) {
            QString pre;
            if (num.numAvailable > 0 || num.numMissingFiles > 0)
                pre = "; ";
            text.append(QString("%1New: %2")
                        .arg(pre, format::filesNumberAndSize(num.numNewFiles, newFilesDataSize)));
        }
    }

    return text;
}

void DataMaintainer::dbStatus()
{
    updateNumbers();

    QString result = "DB filename: " + data_->databaseFileName();

    if (!data_->isWorkDirRelative())
        result.append("\nWorkDir: " + data_->metaData.workDir);

    result.append(QString("\nAlgorithm: %1").arg(format::algoToStr(data_->metaData.algorithm)));

    if (data_->isFilterApplied()) {
        if (data_->metaData.filter.includeOnly)
            result.append(QString("\nIncluded Only: %1").arg(data_->metaData.filter.extensionsList.join(", ")));
        else
            result.append(QString("\nIgnored: %1").arg(data_->metaData.filter.extensionsList.join(", ")));
    }

    result.append(QString("\nLast update: %1").arg(data_->metaData.saveDateTime));

    if (data_->numbers.numChecksums != data_->numbers.numAvailable)
        result.append(QString("\n\nStored checksums: %1").arg(data_->numbers.numChecksums));
    else
        result.append("\n");

    if (data_->numbers.numAvailable > 0)
        result.append(QString("\nAvailable: %1").arg(format::filesNumberAndSize(data_->numbers.numAvailable, data_->numbers.totalSize)));
    else
        result.append("\nNO FILES available to check");

    if (data_->numbers.numUnreadable > 0)
        result.append(QString("\nUnreadable files: %1").arg(data_->numbers.numUnreadable));

    if (data_->numbers.numNewFiles > 0)
        result.append("\n\nNew: " + Files::itemInfo(data_->model_, {FileStatus::New}));
    else
        result.append("\n\nNo New files found");

    if (data_->numbers.numMissingFiles > 0)
        result.append(QString("\nMissing: %1 files").arg(data_->numbers.numMissingFiles));
    else
        result.append("\nNo Missing files found");

    if (data_->numbers.numAvailable > 0) {
        if (data_->numbers.numNotChecked == 0) {
            if (data_->numbers.numMismatched > 0)
                result.append(QString("\n\n☒ %1 mismatches of %2 checksums").arg(data_->numbers.numMismatched).arg(data_->numbers.numChecksums));
            else if (data_->numbers.numChecksums == data_->numbers.numAvailable)
                result.append(QString("\n\n✓ ALL %1 stored checksums matched").arg(data_->numbers.numChecksums));
            else
                result.append(QString("\n\n✓ All %1 available files matched the stored checksums").arg(data_->numbers.numAvailable));
        }
        else if (data_->numbers.numNewFiles > 0 || data_->numbers.numMissingFiles > 0)
            result.append("\n\nUse context menu for more options");
    }

    emit showMessage(result, "Database status");
}

bool DataMaintainer::makeBackup(bool forceOverwrite)
{
    if (!data_ || !QFile::exists(data_->metaData.databaseFilePath))
        return false;

    if (forceOverwrite)
        removeBackupFile();

    return QFile::copy(data_->metaData.databaseFilePath,
                       paths::backupFilePath(data_->metaData.databaseFilePath));
}

void DataMaintainer::removeBackupFile()
{
    if (!data_)
        return;

    QString backupFilePath = paths::backupFilePath(data_->metaData.databaseFilePath);

    if (QFile::exists(backupFilePath))
        QFile::remove(backupFilePath);
}

QModelIndex DataMaintainer::sourceIndex(const QModelIndex &curIndex)
{
    if (!data_ || !curIndex.isValid())
        return QModelIndex();

    if (curIndex.model() == data_->proxyModel_)
        return data_->proxyModel_->mapToSource(curIndex);

    return curIndex;
}

void DataMaintainer::cancelProcess()
{
    canceled = true;
}

DataMaintainer::~DataMaintainer()
{
    removeBackupFile();
    clearData();
    emit setPermanentStatus();
    qDebug() << "DataMaintainer deleted";
}
