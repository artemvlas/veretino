/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "datamaintainer.h"
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QDirIterator>
#include "treemodeliterator.h"

DataMaintainer::DataMaintainer(QObject *parent)
    : QObject(parent)
{
    connections();
}

DataMaintainer::DataMaintainer(DataContainer* initData, QObject *parent)
    : QObject(parent)
{
    connections();
    setSourceData(initData);
}

void DataMaintainer::connections()
{
    connect(this, &DataMaintainer::cancelProcess, this, [=]{canceled = true;}, Qt::DirectConnection);

    // JsonDb
    connect(this, &DataMaintainer::cancelProcess, json, &JsonDb::cancelProcess, Qt::DirectConnection);
    connect(json, &JsonDb::setStatusbarText, this, &DataMaintainer::setStatusbarText);
    connect(json, &JsonDb::showMessage, this, &DataMaintainer::showMessage);
}

void DataMaintainer::setSourceData()
{
    setSourceData(new DataContainer(this));
}

bool DataMaintainer::setSourceData(DataContainer *sourceData)
{
    if (sourceData) {
        clearOldData();
        oldData_ = data_;
        data_ = sourceData;
        data_->setParent(this);
        updateNumbers();
    }

    return sourceData; // false if sourceData == nullptr, else true
}

void DataMaintainer::clearData()
{
    if (data_) {
        delete data_;
        data_ = nullptr;
    }
}

void DataMaintainer::clearOldData()
{
    if (oldData_) {
        delete oldData_;
        oldData_ = nullptr;
    }
}

void DataMaintainer::updateSuccessfulCheckDateTime()
{
    if (data_
        && !data_->contains(FileStatus::Missing)
        && json->updateSuccessfulCheckDateTime(data_->metaData.databaseFilePath)) {

        data_->metaData.successfulCheckDateTime = format::currentDateTime();
    }
}

// add new files to data_->model_
int DataMaintainer::addActualFiles(FileStatus addedFileStatus, bool ignoreUnreadable, bool finalProcess)
{
    if (!data_)
        return 0;

    if (!QFileInfo(data_->metaData.workDir).isDir()) {
        qDebug() << "DataMaintainer::addActualFiles | Not a folder path: " << data_->metaData.workDir;
        return 0;
    }

    emit processing(true);
    emit setStatusbarText("Creating a list of files...");

    canceled = false;
    int numAdded = 0;

    QDir dir(data_->metaData.workDir);
    QDirIterator it(data_->metaData.workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !canceled) {
        QString fullPath = it.next();
        QString relPath = dir.relativeFilePath(fullPath);

        if (data_->metaData.filter.isFileAllowed(relPath)) {
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

    if (finalProcess || canceled)
        emit processing(false);

    if (canceled) {
        qDebug() << "DataMaintainer::addActualFiles | Canceled:" << data_->metaData.workDir;
        clearData();
        emit setStatusbarText();
        return 0;
    }

    if (numAdded > 0)
        updateNumbers();

    return numAdded;
}

void DataMaintainer::updateNumbers()
{
    if (!data_) {
        qDebug() << "DataMaintainer::updateNumbers | NO data_";
        return;
    }

    emit setPermanentStatus("updating...");
    data_->numbers = updateNumbers(data_->model_);

    if (data_->contains({FileStatus::Mismatched, FileStatus::Missing}))
        data_->metaData.successfulCheckDateTime.clear();

    QString newmissing;
    QString mismatched;
    QString matched;
    QString sep;

    if (data_->numbers.numberOf(FileStatus::New) > 0 || data_->numbers.numberOf(FileStatus::Missing) > 0)
        newmissing = "* ";

    if (data_->numbers.numberOf(FileStatus::Mismatched) > 0)
        mismatched = QString("☒%1").arg(data_->numbers.numberOf(FileStatus::Mismatched));
    if (data_->numbers.numberOf(FileStatus::Matched) > 0)
        matched = QString(" ✓%1").arg(data_->numbers.numberOf(FileStatus::Matched)
                                      + data_->numbers.numberOf(FileStatus::Added)
                                      + data_->numbers.numberOf(FileStatus::ChecksumUpdated));

    if (data_->numbers.numberOf(FileStatus::Mismatched) > 0 || data_->numbers.numberOf(FileStatus::Matched) > 0)
        sep = " : ";

    QString checkStatus = QString("\t%1%2%3%4").arg(newmissing, mismatched, matched, sep);

    emit setPermanentStatus(QString("%1%2 avail. | %3 | %4")
                            .arg(checkStatus)
                            .arg(data_->numbers.available())
                            .arg(format::dataSizeReadable(data_->numbers.totalSize), format::algoToStr(data_->metaData.algorithm)));
}

Numbers DataMaintainer::updateNumbers(const QAbstractItemModel *model, const QModelIndex &rootIndex)
{
    Numbers num;

    TreeModelIterator iter(model, rootIndex);

    while (iter.hasNext()) {
        iter.nextFile();

        if (iter.data(Column::ColumnChecksum).isValid()
            && !iter.data(Column::ColumnChecksum).toString().isEmpty()) {

            ++num.numChecksums;
            num.totalSize += iter.data(Column::ColumnSize).toLongLong();
        }

        if (!num.holder.contains(iter.status())) {
            num.holder.insert(iter.status(), 1);
        }
        else {
            int storedNumber = num.holder.value(iter.status());
            num.holder.insert(iter.status(), ++storedNumber);
        }
    }

    //qDebug() << "DataMaintainer::updateNumbers |" << num.holder;

    return num;
}

qint64 DataMaintainer::totalSizeOfListedFiles(const FileStatus fileStatus, const QModelIndex &rootIndex)
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
        QVariant itData = it.nextFile().data(Column::ColumnStatus);

        if (itData.isValid()
            && (fileStatuses.isEmpty()
                || fileStatuses.contains(static_cast<FileStatus>(itData.toInt())))) {

            result += it.data(Column::ColumnSize).toLongLong();
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

    QString storedChecksum = TreeModel::siblingAtRow(fileRowIndex, Column::ColumnChecksum).data().toString();

    if (storedChecksum.isEmpty()) {
        data_->model_->setRowData(fileRowIndex, Column::ColumnChecksum, computedChecksum);
        data_->model_->setRowData(fileRowIndex, Column::ColumnStatus, FileStatus::Added);
    }
    else if (storedChecksum == computedChecksum)
        data_->model_->setRowData(fileRowIndex, Column::ColumnStatus, FileStatus::Matched);
    else {
        data_->model_->setRowData(fileRowIndex, Column::ColumnReChecksum, computedChecksum);
        data_->model_->setRowData(fileRowIndex, Column::ColumnStatus, FileStatus::Mismatched);
    }

    return (storedChecksum.isEmpty() || storedChecksum == computedChecksum);
}

int DataMaintainer::changeFilesStatus(const FileStatus currentStatus, const FileStatus newStatus, const QModelIndex &rootIndex)
{
    return changeFilesStatus(QSet<FileStatus>({currentStatus}), newStatus, rootIndex);
}

int DataMaintainer::changeFilesStatus(const QSet<FileStatus> curStatuses, const FileStatus newStatus, const QModelIndex &rootIndex)
{
    if (!data_) {
        qDebug() << "DataMaintainer::changeFilesStatus | NO data_";
        return 0;
    }

    if (newStatus == FileStatus::Queued)
        qDebug() << "adding to queue...";

    int number = 0;
    TreeModelIterator iter(data_->model_, rootIndex);

    while (iter.hasNext()) {
        if (curStatuses.contains(iter.nextFile().status())) {
            data_->model_->setRowData(iter.index(), Column::ColumnStatus, newStatus);
            ++number;
        }
    }

    if (number > 0)
        updateNumbers();

    if (newStatus == FileStatus::Queued)
        qDebug() << QString("%1 files added to queue").arg(number);

    return number;
}

int DataMaintainer::addToQueue(const FileStatus currentStatus, const QModelIndex &rootIndex)
{
    return changeFilesStatus(currentStatus, FileStatus::Queued, rootIndex);
}

int DataMaintainer::addToQueue(const QSet<FileStatus> curStatuses, const QModelIndex &rootIndex)
{
    return changeFilesStatus(curStatuses, FileStatus::Queued, rootIndex);
}

int DataMaintainer::clearChecksums(const FileStatus curStatus, const QModelIndex &rootIndex)
{
    return clearChecksums(QSet<FileStatus>({curStatus}), rootIndex);
}

int DataMaintainer::clearChecksums(const QSet<FileStatus> curStatuses, const QModelIndex &rootIndex)
{
    if (!data_) {
        qDebug() << "DataMaintainer::clearChecksums | NO data_";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(data_->model_, rootIndex);

    while (iter.hasNext()) {
        if (curStatuses.contains(iter.nextFile().status())) {
            data_->model_->setRowData(iter.index(), Column::ColumnChecksum);
            ++number;
        }
    }

    return number;
}

QString DataMaintainer::getStoredChecksum(const QModelIndex &fileRowIndex)
{
    if (!data_) {
        qDebug() << "DataMaintainer::getStoredChecksum | NO data_";
        return QString();
    }

    if (!TreeModel::isFileRow(fileRowIndex)) {
        qDebug() << "DataMaintainer::getStoredChecksum | Specified index does not belong to the file row";
        return QString();
    }

    FileStatus fileStatus = TreeModel::siblingAtRow(fileRowIndex, TreeModel::ColumnStatus)
                                        .data(TreeModel::RawDataRole).value<FileStatus>();

    QString savedSum;

    if (fileStatus == FileStatus::New)
        emit showMessage("The checksum is not yet in the database.\nPlease Update New/Lost", "NEW File");
    else if (fileStatus == FileStatus::Unreadable)
        emit showMessage("This file has been excluded (Unreadable).\nNo checksum in the database.", "Excluded File");
    else {
        savedSum = TreeModel::siblingAtRow(fileRowIndex, Column::ColumnChecksum).data().toString();
        if (savedSum.isEmpty())
            emit showMessage("No checksum in the database.", "No checksum");
    }

    return savedSum;
}

int DataMaintainer::clearDataFromLostFiles(bool finalProcess)
{
    if (!data_) {
        qDebug() << "DataMaintainer::clearDataFromLostFiles | NO data_";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(data_->model_);

    emit processing(true);

    while (iter.hasNext()) {
        if (iter.nextFile().status() == FileStatus::Missing) {
            data_->model_->setRowData(iter.index(), Column::ColumnChecksum);
            data_->model_->setRowData(iter.index(), Column::ColumnStatus, FileStatus::Removed);
            ++number;
        }
    }

    if (number > 0)
        updateNumbers();

    if (finalProcess)
        emit processing(false);

    return number;
}

int DataMaintainer::updateMismatchedChecksums(bool finalProcess)
{
    if (!data_) {
        qDebug() << "DataMaintainer::updateMismatchedChecksums | NO data_";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(data_->model_);

    emit processing(true);

    while (iter.hasNext()) {
        if (iter.nextFile().status() == FileStatus::Mismatched) {
            QString reChecksum = iter.data(Column::ColumnReChecksum).toString();

            if (!reChecksum.isEmpty()) {
                data_->model_->setRowData(iter.index(), Column::ColumnChecksum, reChecksum);
                data_->model_->setRowData(iter.index(), Column::ColumnReChecksum);
                data_->model_->setRowData(iter.index(), Column::ColumnStatus, FileStatus::ChecksumUpdated);
                ++number;
            }
        }
    }

    if (number > 0)
        updateNumbers();

    if (finalProcess)
        emit processing(false);

    return number;
}

bool DataMaintainer::importJson(const QString &jsonFilePath)
{
    return setSourceData(json->parseJson(jsonFilePath));
}

void DataMaintainer::exportToJson(bool finalProcess)
{
    if (!data_)
        return;

    updateNumbers();
    data_->makeBackup();
    data_->metaData.successfulCheckDateTime.clear();
    data_->metaData.saveDateTime = format::currentDateTime();

    emit processing(true);

    data_->metaData.savingResult = json->makeJson(data_);

    if (finalProcess)
        emit processing(false);

    emit databaseUpdated();
}

QString DataMaintainer::itemContentsInfo(const QModelIndex &curIndex)
{
    QString text;

    if (TreeModel::isFileRow(curIndex)) {
        QString itemFileNameStr = TreeModel::siblingAtRow(curIndex, Column::ColumnPath).data().toString();
        QVariant itemFileSize = TreeModel::siblingAtRow(curIndex, Column::ColumnSize).data(TreeModel::RawDataRole);
        QString itemFileSizeStr = itemFileSize.isValid() ? QString(" (%1)").arg(format::dataSizeReadable(itemFileSize.toLongLong()))
                                                         : QString();

        return QString("%1%2").arg(itemFileNameStr, itemFileSizeStr);
    }
    // if curIndex is at folder row
    else if (curIndex.isValid()) {
        Numbers num = updateNumbers(curIndex.model(), curIndex);
        qint64 newFilesDataSize = totalSizeOfListedFiles(QSet<FileStatus>({FileStatus::New}), curIndex);

        if (num.available() > 0) {
            text = QString("Avail.: %1")
                       .arg(format::filesNumberAndSize(num.available(), num.totalSize - newFilesDataSize));
        }

        if (num.numberOf(FileStatus::Missing) > 0) {
            QString pre;
            if (num.available() > 0)
                pre = "; ";
            text.append(QString("%1Missing: %2").arg(pre).arg(num.numberOf(FileStatus::Missing)));
        }

        if (num.numberOf(FileStatus::New) > 0) {
            QString pre;
            if (num.available() > 0 || num.numberOf(FileStatus::Missing) > 0)
                pre = "; ";
            text.append(QString("%1New: %2")
                        .arg(pre, format::filesNumberAndSize(num.numberOf(FileStatus::New), newFilesDataSize)));
        }
    }

    return text;
}

QModelIndex DataMaintainer::sourceIndex(const QModelIndex &curIndex)
{
    if (!data_ || !curIndex.isValid())
        return QModelIndex();

    if (curIndex.model() == data_->proxyModel_)
        return data_->proxyModel_->mapToSource(curIndex);

    return curIndex;
}

DataMaintainer::~DataMaintainer()
{
    clearData();
    qDebug() << "DataMaintainer deleted";
}
