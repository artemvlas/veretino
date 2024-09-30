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
                                                                  data_->metaData.datetime[DTstr::DateVerified].clear(); });
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

bool DataMaintainer::setItemValue(const QModelIndex &fileIndex, Column column, const QVariant &value)
{
    return (data_ && data_->model_->setData(fileIndex.siblingAtColumn(column), value));
}

void DataMaintainer::setConsiderDateModified(bool consider)
{
    json_->considerFileModDate = consider;
}

void DataMaintainer::updateDateTime()
{
    if (data_) {
        if (data_->isDbFileState(DbFileState::NoFile)) {
            data_->metaData.datetime[DTstr::DateCreated] = QStringLiteral(u"Created: ") + format::currentDateTime();
        }
        else if (data_->contains(FileStatus::CombDbChanged)) {
            data_->metaData.datetime[DTstr::DateUpdated] = QStringLiteral(u"Updated: ") + format::currentDateTime();
            data_->metaData.datetime[DTstr::DateVerified].clear();
        }
    }
}

void DataMaintainer::updateVerifDateTime()
{
    if (data_ && data_->isAllMatched()) {
        data_->metaData.datetime[DTstr::DateVerified] = QStringLiteral(u"Verified: ") + format::currentDateTime();
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

    emit setStatusbarText(QStringLiteral(u"Creating a list of files..."));

    int numAdded = 0;

    QDirIterator it(_workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        const QString _fullPath = it.next();
        const QString _relPath = paths::relativePath(_workDir, _fullPath);

        if (data_->metaData.filter.isFileAllowed(_relPath)) {
            QFileInfo _fileInfo(_fullPath);
            const bool _isReadable = _fileInfo.isReadable();

            if (!_isReadable && ignoreUnreadable)
                continue;

            const FileValues &_values = _isReadable ? FileValues(fileStatus, _fileInfo.size())
                                                    : FileValues(FileStatus::UnPermitted);

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

    QString storedChecksum = TreeModel::itemFileChecksum(fileRowIndex);

    if (storedChecksum.isEmpty()) {
        setItemValue(fileRowIndex, Column::ColumnChecksum, computedChecksum);
        setItemValue(fileRowIndex, Column::ColumnStatus, FileStatus::Added);
        return true;
    }
    else if (storedChecksum == computedChecksum) {
        setItemValue(fileRowIndex, Column::ColumnStatus, FileStatus::Matched);
        return true;
    }
    else {
        setItemValue(fileRowIndex, Column::ColumnReChecksum, computedChecksum);
        setItemValue(fileRowIndex, Column::ColumnStatus, FileStatus::Mismatched);
        return false;
    }
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
            setItemValue(iter.index(), Column::ColumnStatus, newStatus);
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
            setItemValue(iter.index(), Column::ColumnChecksum);
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
        setItemValue(fileIndex, Column::ColumnChecksum);
        setItemValue(fileIndex, Column::ColumnStatus, FileStatus::Removed);
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
            setItemValue(fileIndex, Column::ColumnChecksum, reChecksum);
            setItemValue(fileIndex, Column::ColumnReChecksum);
            setItemValue(fileIndex, Column::ColumnStatus, FileStatus::Updated);
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

        changeFilesStatus((FileStatus::CombProcessing | FileStatus::Added), status, rootIndex);
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

    QString _dbFilePath = json_->makeJson(data_);

    if (!_dbFilePath.isEmpty()) {
        setDbFileState(data_->isInCreation() ? DbFileState::Created : DbFileState::Saved);
        data_->metaData.dbFilePath = _dbFilePath;
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

    if (TreeModel::isFileRow(curIndex)) {
        const QString _fileName = TreeModel::itemName(curIndex);
        const QVariant _fileSize = curIndex.siblingAtColumn(Column::ColumnSize).data(TreeModel::RawDataRole);

        if (_fileSize.isValid()) {
            return format::addStrInParentheses(_fileName,
                                               format::dataSizeReadable(_fileSize.toLongLong()));
        }

        return _fileName;
    }
    // if curIndex is at folder row
    else if (TreeModel::isFolderRow(curIndex)) {
        const Numbers &num = data_->getNumbers(curIndex);
        QStringList _sl;

        if (num.contains(FileStatus::CombAvailable)) {
            QString __s = format::filesNumSize(num, FileStatus::CombAvailable);
            _sl << QStringLiteral(u"Avail.: ") + __s;
        }

        if (num.contains(FileStatus::New)) {
            QString __s = format::filesNumSize(num, FileStatus::New);
            _sl << QStringLiteral(u"New: ") + __s;
        }

        if (num.contains(FileStatus::Missing)) {
            _sl << tools::joinStrings(QStringLiteral(u"Missing:"), num.numberOf(FileStatus::Missing));
        }

        if (num.contains(FileStatus::CombUnreadable)) {
            _sl << tools::joinStrings(QStringLiteral(u"Unread.:"), num.numberOf(FileStatus::CombUnreadable));
        }

        return _sl.join(QStringLiteral(u"; "));
    }

    return QString();
}

bool DataMaintainer::isCanceled() const
{
    return (proc_ && proc_->isCanceled());
}

bool DataMaintainer::isDataNotSaved() const
{
    return (data_ && data_->isDbFileState(DbFileState::NotSaved));
}

void DataMaintainer::saveData()
{
    if (isDataNotSaved()) {
        exportToJson();
    }
}

DataMaintainer::~DataMaintainer()
{
    saveData(); // insurance
    qDebug() << Q_FUNC_INFO;
}
