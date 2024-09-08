/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "datamaintainer.h"
#include <QFileInfo>
#include <QDebug>
#include <QDirIterator>
#include "treemodeliterator.h"
#include "tools.h"

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
    // JsonDb
    connect(json_, &JsonDb::setStatusbarText, this, &DataMaintainer::setStatusbarText);
    connect(json_, &JsonDb::showMessage, this, &DataMaintainer::showMessage);

    connect(this, &DataMaintainer::numbersUpdated, this, [=]{ if (data_->contains(FileStatus::Mismatched | FileStatus::Missing))
                                                                  data_->metaData.datetime[DateTimeStr::DateVerified].clear(); });
}

void DataMaintainer::setProcState(const ProcState *procState)
{
    proc_ = procState;
    json_->setProcState(procState);
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

void DataMaintainer::updateDateTime()
{
    if (data_) {
        if (data_->isDbFileState(DbFileState::NoFile)) {
            data_->metaData.datetime[DateTimeStr::DateCreated] = "Created: " + format::currentDateTime();
        }
        else if (data_->contains(FileStatus::FlagDbChanged)) {
            data_->metaData.datetime[DateTimeStr::DateUpdated] = "Updated: " + format::currentDateTime();
            data_->metaData.datetime[DateTimeStr::DateVerified].clear();
        }
    }
}

void DataMaintainer::updateVerifDateTime()
{
    if (data_ && data_->isAllMatched()) {
        data_->metaData.datetime[DateTimeStr::DateVerified] = "Verified: " + format::currentDateTime();
        setDbFileState(DbFileState::NotSaved);
    }
}

// adds the WorkDir contents to the data_->model_
int DataMaintainer::addActualFiles(FileStatus fileStatus, bool ignoreUnreadable)
{
    if (!data_)
        return 0;

    const QString &_workDir = data_->metaData.workDir;

    if (!QFileInfo(_workDir).isDir()) {
        qDebug() << "DataMaintainer::addActualFiles | Not a folder path: " << _workDir;
        return 0;
    }

    emit setStatusbarText("Creating a list of files...");

    int numAdded = 0;

    QDirIterator it(_workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        const QString &_fullPath = it.next();
        const QString &_relPath = paths::relativePath(_workDir, _fullPath);

        if (data_->metaData.filter.isFileAllowed(_relPath)) {
            QFileInfo _fileInfo(_fullPath);
            const bool _isReadable = _fileInfo.isReadable();

            if (!_isReadable && ignoreUnreadable)
                continue;

            const FileValues &_values = _isReadable ? FileValues(fileStatus, _fileInfo.size())
                                                    : FileValues(FileStatus::Unreadable);

            data_->model_->add_file(_relPath, _values);
            ++numAdded;
        }
    }

    if (isCanceled()) {
        qDebug() << "DataMaintainer::addActualFiles | Canceled:" << _workDir;
        clearData();
        emit setStatusbarText();
        return 0;
    }

    if (numAdded > 0)
        updateNumbers();

    data_->model_->clearCacheFolderItems();
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
}

// if only one file has changed, there is no need to iterate over the entire list
void DataMaintainer::updateNumbers(const QModelIndex &fileIndex, const FileStatus statusBefore)
{
    if (data_
        && data_->numbers.moveFile(statusBefore,
                                   TreeModel::itemFileStatus(fileIndex),
                                   TreeModel::itemFileSize(fileIndex)))
    {
        emit numbersUpdated();
    }
}

void DataMaintainer::setDbFileState(DbFileState state)
{
    if (data_ && !data_->isDbFileState(state)) {
        data_->metaData.dbFileState = state;
        emit dbFileStateChanged(state == DbFileState::NotSaved);
    }
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

    return result;
}

int DataMaintainer::changeFilesStatus(const FileStatuses flags, const FileStatus newStatus, const QModelIndex &rootIndex)
{
    if (!data_) {
        qDebug() << "DataMaintainer::changeFilesStatus | NO data_";
        return 0;
    }

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

int DataMaintainer::clearLostFiles()
{
    if (!data_) {
        qDebug() << "DataMaintainer::clearLostFiles | NO data_";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(data_->model_);

    while (iter.hasNext()) {
        if (itemFileRemoveLost(iter.nextFile().index()))
            ++number;
    }

    if (number > 0) {
        setDbFileState(DbFileState::NotSaved);
        updateNumbers();
    }

    return number;
}

bool DataMaintainer::itemFileRemoveLost(const QModelIndex &fileIndex)
{
    if (data_ && TreeModel::hasStatus(FileStatus::Missing, fileIndex)) {
        data_->model_->setRowData(fileIndex, Column::ColumnChecksum);
        data_->model_->setRowData(fileIndex, Column::ColumnStatus, FileStatus::Removed);
        return true;
    }

    return false;
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
        if (itemFileUpdateChecksum(iter.nextFile().index()))
            ++number;
    }

    if (number > 0) {
        setDbFileState(DbFileState::NotSaved);
        updateNumbers();
    }

    return number;
}

bool DataMaintainer::itemFileUpdateChecksum(const QModelIndex &fileIndex)
{
    if (data_ && TreeModel::hasReChecksum(fileIndex)) {
        QString reChecksum = TreeModel::itemFileReChecksum(fileIndex);

        if (!reChecksum.isEmpty()) {
            data_->model_->setRowData(fileIndex, Column::ColumnChecksum, reChecksum);
            data_->model_->setRowData(fileIndex, Column::ColumnReChecksum);
            data_->model_->setRowData(fileIndex, Column::ColumnStatus, FileStatus::Updated);
            return true;
        }
    }

    return false;
}

void DataMaintainer::rollBackStoppedCalc(const QModelIndex &rootIndex, FileStatus status)
{
    if (status != FileStatus::Queued) { // data_ && !data_->isInCreation()
        if (status == FileStatus::New)
            clearChecksums(FileStatus::Added, rootIndex);

        changeFilesStatus((FileStatus::FlagProcessing | FileStatus::Added), status, rootIndex);
    }
}

bool DataMaintainer::importJson(const QString &jsonFilePath)
{
    return setSourceData(json_->parseJson(jsonFilePath));
}

void DataMaintainer::exportToJson()
{
    if (!data_)
        return;

    data_->makeBackup();

    QString dbFilePath = json_->makeJson(data_);

    if (!dbFilePath.isEmpty()) {
        setDbFileState(data_->isInCreation() ? DbFileState::Created : DbFileState::Saved);
        data_->metaData.databaseFilePath = dbFilePath;
    }
    else {
        setDbFileState(DbFileState::NotSaved);
    }

    // debug info
    if (data_->isDbFileState(DbFileState::Saved))
        qDebug() << "DataMaintainer::exportToJson >> Saved";
}

void DataMaintainer::forkJsonDb(const QModelIndex &rootFolder)
{
    if (!data_)
        return;

    if (!TreeModel::isFolderRow(rootFolder)) {
        qDebug() << "DataMaintainer::forkJsonDb | wrong folder index";
        return;
    }

    emit subDbForked(json_->makeJson(data_, rootFolder));
}

QString DataMaintainer::itemContentsInfo(const QModelIndex &curIndex)
{
    if (!data_ || !curIndex.isValid() || (curIndex.model() != data_->model_))
        return QString();

    QString text;

    if (TreeModel::isFileRow(curIndex)) {
        QString itemFileNameStr = TreeModel::itemName(curIndex);
        QVariant itemFileSize = curIndex.siblingAtColumn(Column::ColumnSize).data(TreeModel::RawDataRole);
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

bool DataMaintainer::isCanceled() const
{
    return proc_ && proc_->isCanceled();
}

bool DataMaintainer::isDataNotSaved() const
{
    return (data_ && data_->isDbFileState(DbFileState::NotSaved));
}

void DataMaintainer::saveData()
{
    if (isDataNotSaved())
        exportToJson();
}

DataMaintainer::~DataMaintainer()
{
    saveData(); // insurance
    qDebug() << Q_FUNC_INFO;
}
