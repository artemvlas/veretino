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
    connect(this, &DataMaintainer::cancelProcess, this, [=]{ canceled = true; }, Qt::DirectConnection);

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
int DataMaintainer::addActualFiles(FileStatus fileStatus, bool ignoreUnreadable)
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

        if (data_->metaData.filter.isFileAllowed(relPath)) {
            FileValues curFileValues;
            QFileInfo fileInfo(fullPath);
            if (fileInfo.isReadable()) {
                curFileValues.status = fileStatus;
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

    data_->updateNumbers();
    emit numbersUpdated();

    if (data_->contains(FileStatus::Mismatched | FileStatus::Missing))
        data_->metaData.successfulCheckDateTime.clear();
}

qint64 DataMaintainer::totalSizeOfListedFiles(const FileStatuses flags, const QModelIndex &rootIndex)
{
    if (!data_) {
        qDebug() << "DataMaintainer::totalSizeOfListedFiles | NO data_";
        return 0;
    }

    if (rootIndex.isValid() && rootIndex.model() != data_->model_) {
        qDebug() << "DataMaintainer::totalSizeOfListedFiles | Wrong model index";
        return 0;
    }

    qint64 result = 0;
    TreeModelIterator it(data_->model_, rootIndex);

    while (it.hasNext()) {
        if (flags & it.nextFile().status()) {
            result += it.size();
        }
    }

    return result;
}

bool DataMaintainer::updateChecksum(const QModelIndex &fileRowIndex, const QString &computedChecksum)
{
    if (!data_ || !fileRowIndex.isValid() || fileRowIndex.model() != data_->model_) {
        qDebug() << "DataMaintainer::updateChecksum | NO data_ or invalid index";
        return false;
    }

    bool result = false;
    QString storedChecksum = TreeModel::itemFileChecksum(fileRowIndex);

    if (storedChecksum.isEmpty()) {
        data_->model_->setRowData(fileRowIndex, Column::ColumnChecksum, computedChecksum);
        data_->model_->setRowData(fileRowIndex, Column::ColumnStatus, FileStatus::Added);
        result = true;
    }
    else if (storedChecksum == computedChecksum) {
        data_->model_->setRowData(fileRowIndex, Column::ColumnStatus, FileStatus::Matched);
        result = true;
    }
    else {
        data_->model_->setRowData(fileRowIndex, Column::ColumnReChecksum, computedChecksum);
        data_->model_->setRowData(fileRowIndex, Column::ColumnStatus, FileStatus::Mismatched);
    }

    return (result);
}


int DataMaintainer::changeFilesStatus(const FileStatuses flags, const FileStatus newStatus, const QModelIndex &rootIndex)
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
        if (flags & iter.nextFile().status()) {
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

int DataMaintainer::addToQueue(const FileStatuses flags, const QModelIndex &rootIndex)
{
    return changeFilesStatus(flags, FileStatus::Queued, rootIndex);
}

int DataMaintainer::clearChecksums(const FileStatuses flags, const QModelIndex &rootIndex)
{
    if (!data_) {
        qDebug() << "DataMaintainer::clearChecksums | NO data_";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(data_->model_, rootIndex);

    while (iter.hasNext()) {
        if (flags & iter.nextFile().status()) {
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

    FileStatus fileStatus = TreeModel::itemFileStatus(fileRowIndex);

    QString savedSum;

    if (fileStatus == FileStatus::New)
        emit showMessage("The checksum is not yet in the database.\nPlease Update New/Lost", "NEW File");
    else if (fileStatus == FileStatus::Unreadable)
        emit showMessage("This file has been excluded (Unreadable).\nNo checksum in the database.", "Excluded File");
    else {
        savedSum = TreeModel::itemFileChecksum(fileRowIndex);
        if (savedSum.isEmpty())
            emit showMessage("No checksum in the database.", "No checksum");
    }

    return savedSum;
}

int DataMaintainer::clearLostFiles()
{
    if (!data_) {
        qDebug() << "DataMaintainer::clearLostFiles | NO data_";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(data_->model_);

    while (iter.hasNext()) {
        if (iter.nextFile().status() == FileStatus::Missing) {
            data_->model_->setRowData(iter.index(), Column::ColumnChecksum);
            data_->model_->setRowData(iter.index(), Column::ColumnStatus, FileStatus::Removed);
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
            QString reChecksum = iter.data(Column::ColumnReChecksum).toString();

            if (!reChecksum.isEmpty()) {
                data_->model_->setRowData(iter.index(), Column::ColumnChecksum, reChecksum);
                data_->model_->setRowData(iter.index(), Column::ColumnReChecksum);
                data_->model_->setRowData(iter.index(), Column::ColumnStatus, FileStatus::Updated);
                ++number;
            }
        }
    }

    if (number > 0)
        updateNumbers();

    return number;
}

bool DataMaintainer::importJson(const QString &jsonFilePath)
{
    return setSourceData(json->parseJson(jsonFilePath));
}

void DataMaintainer::exportToJson()
{
    if (!data_)
        return;

    data_->makeBackup();
    data_->metaData.successfulCheckDateTime.clear();
    data_->metaData.saveDateTime = format::currentDateTime();

    data_->setSaveResult(json->makeJson(data_));

    emit databaseUpdated();
}

void DataMaintainer::forkJsonDb(const QModelIndex &rootFolder)
{
    if (!data_)
        return;

    if (!TreeModel::isFolderRow(rootFolder)) {
        qDebug() << "DataMaintainer::forkJsonDb | wrong folder index";
        return;
    }

    emit subDbForked(json->makeJson(data_, rootFolder));
}

QString DataMaintainer::itemContentsInfo(const QModelIndex &curIndex)
{
    if (!data_ || !curIndex.isValid() || (curIndex.model() != data_->model_))
        return QString();

    QString text;

    if (TreeModel::isFileRow(curIndex)) {
        QString itemFileNameStr = TreeModel::itemName(curIndex);
        QVariant itemFileSize = TreeModel::siblingAtRow(curIndex, Column::ColumnSize).data(TreeModel::RawDataRole);
        QString itemFileSizeStr = itemFileSize.isValid() ? QString(" (%1)").arg(format::dataSizeReadable(itemFileSize.toLongLong()))
                                                         : QString();

        return QString("%1%2").arg(itemFileNameStr, itemFileSizeStr);
    }
    // if curIndex is at folder row
    else if (TreeModel::isFolderRow(curIndex)) {
        const Numbers num = data_->getNumbers(curIndex);
        const bool containsAvailable = num.contains(FileStatus::FlagAvailable);

        if (containsAvailable) {
            text = QString("Avail.: %1")
                       .arg(format::filesNumberAndSize(num.numberOf(FileStatus::FlagAvailable), num.totalSize(FileStatus::FlagAvailable)));
        }

        if (num.contains(FileStatus::Missing)) {
            QString pre;
            if (containsAvailable)
                pre = "; ";
            text.append(QString("%1Missing: %2").arg(pre).arg(num.numberOf(FileStatus::Missing)));
        }

        if (num.contains(FileStatus::New)) {
            QString pre;
            if (containsAvailable || num.contains(FileStatus::Missing))
                pre = "; ";
            text.append(QString("%1New: %2")
                        .arg(pre, format::filesNumberAndSize(num.numberOf(FileStatus::New), num.totalSize(FileStatus::New))));
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
