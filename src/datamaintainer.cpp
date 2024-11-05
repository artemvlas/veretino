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
                                                                  data_->metaData_.datetime[DTstr::DateVerified].clear(); });
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

void DataMaintainer::setSourceData(const MetaData &meta)
{
    setSourceData(new DataContainer(meta, this));
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

void DataMaintainer::setFileStatus(const QModelIndex &_index, FileStatus _status)
{
    setItemValue(_index, Column::ColumnStatus, _status);
}

void DataMaintainer::setConsiderDateModified(bool consider)
{
    json_->considerFileModDate = consider;
}

void DataMaintainer::updateDateTime()
{
    if (data_) {
        if (data_->isDbFileState(DbFileState::NoFile)) {
            data_->metaData_.datetime[DTstr::DateCreated] = QStringLiteral(u"Created: ") + format::currentDateTime();
        }
        else if (data_->contains(FileStatus::CombDbChanged)) {
            data_->metaData_.datetime[DTstr::DateUpdated] = QStringLiteral(u"Updated: ") + format::currentDateTime();
            data_->metaData_.datetime[DTstr::DateVerified].clear();
        }
    }
}

void DataMaintainer::updateVerifDateTime()
{
    if (data_ && data_->isAllMatched()) {
        data_->metaData_.datetime[DTstr::DateVerified] = QStringLiteral(u"Verified: ") + format::currentDateTime();
        setDbFileState(DbFileState::NotSaved);
    }
}

// adds the WorkDir contents to the data_->model_
int DataMaintainer::folderBasedData(FileStatus fileStatus, bool ignoreUnreadable)
{
    if (!data_)
        return 0;

    const QString &_workDir = data_->metaData_.workDir;

    if (!QFileInfo(_workDir).isDir()) {
        qDebug() << "DM::folderBasedData | Wrong path:" << _workDir;
        return 0;
    }

    emit setStatusbarText(QStringLiteral(u"Creating a list of files..."));

    int numAdded = 0;

    QDirIterator it(_workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        const QString _fullPath = it.next();
        const QString _relPath = paths::relativePath(_workDir, _fullPath);

        if (data_->metaData_.filter.isFileAllowed(_relPath)) {
            const bool _isReadable = it.fileInfo().isReadable();

            if (!_isReadable && ignoreUnreadable)
                continue;

            const FileValues &_values = _isReadable ? FileValues(fileStatus, it.fileInfo().size())
                                                    : FileValues(FileStatus::UnPermitted);

            data_->model_->add_file(_relPath, _values);
            ++numAdded;
        }
    }

    if (isCanceled() || numAdded == 0) {
        qDebug() << "DM::folderBasedData | Canceled/No items:" << _workDir;
        emit setStatusbarText();
        emit failedDataCreation();
        clearData();
        return 0;
    }

    //if (numAdded > 0)
    updateNumbers();

    data_->model_->clearCacheFolderItems();
    return numAdded;
}

void DataMaintainer::updateNumbers()
{
    if (!data_) {
        qDebug() << "DM::updateNumbers | NO data_";
        return;
    }

    data_->updateNumbers();
    emit numbersUpdated();
}

// if only one file has changed, there is no need to iterate over the entire list
void DataMaintainer::updateNumbers(const QModelIndex &fileIndex, const FileStatus statusBefore)
{
    /*if (data_
        && data_->numbers_.moveFile(statusBefore,
                                    TreeModel::itemFileStatus(fileIndex),
                                    TreeModel::itemFileSize(fileIndex)))
    {
        emit numbersUpdated();
    }*/
    updateNumbers(statusBefore,
                  TreeModel::itemFileStatus(fileIndex),
                  TreeModel::itemFileSize(fileIndex));
}

void DataMaintainer::updateNumbers(const FileStatus status_old, const FileStatus status_new, const qint64 size)
{
    if (data_
        && data_->numbers_.moveFile(status_old, status_new, size))
    {
        emit numbersUpdated();
    }
}

void DataMaintainer::moveNumbers(const FileStatus _before, const FileStatus _after)
{
    if (data_ && data_->numbers_.changeStatus(_before, _after))
        emit numbersUpdated();
}

void DataMaintainer::setDbFileState(DbFileState state)
{
    if (data_ && !data_->isDbFileState(state)) {
        data_->metaData_.dbFileState = state;
        emit dbFileStateChanged(state == DbFileState::NotSaved);
    }
}

bool DataMaintainer::updateChecksum(const QModelIndex &fileRowIndex, const QString &computedChecksum)
{
    if (!data_ || !fileRowIndex.isValid() || fileRowIndex.model() != data_->model_) {
        qDebug() << "DM::updateChecksum >> Error";
        return false;
    }

    const QString storedChecksum = TreeModel::itemFileChecksum(fileRowIndex);

    if (storedChecksum.isEmpty()) {
        if (!tryMoved(fileRowIndex, computedChecksum)) {
            setItemValue(fileRowIndex, Column::ColumnChecksum, computedChecksum);
            setFileStatus(fileRowIndex, FileStatus::Added);
        }
        return true;
    }
    else if (storedChecksum == computedChecksum) {
        setFileStatus(fileRowIndex, FileStatus::Matched);
        return true;
    }
    else {
        setItemValue(fileRowIndex, Column::ColumnReChecksum, computedChecksum);
        setFileStatus(fileRowIndex, FileStatus::Mismatched);
        return false;
    }
}

bool DataMaintainer::importChecksum(const QModelIndex &_file, const QString &_checksum)
{
    if (TreeModel::hasStatus(FileStatus::New, _file) && !_checksum.isEmpty()) {
        setItemValue(_file, Column::ColumnChecksum, _checksum);
        setFileStatus(_file, FileStatus::Imported);
        return true;

        // data_->numbers_.moveFile(FileStatus::New, FileStatus::Imported, TreeModel::itemFileSize(_file));
    }

    return false;
}

int DataMaintainer::changeFilesStatus(const FileStatuses flags, const FileStatus newStatus, const QModelIndex &rootIndex)
{
    if (!data_ || tools::isFlagCombined(newStatus)) {
        qDebug() << "DM::changeFilesStatus >> Error";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(data_->model_, rootIndex);

    while (iter.hasNext()) {
        if (flags & iter.nextFile().status()) {
            setFileStatus(iter.index(), newStatus);
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

void DataMaintainer::clearChecksum(const QModelIndex &fileIndex)
{
    setItemValue(fileIndex, Column::ColumnChecksum);
}

int DataMaintainer::clearChecksums(const FileStatuses flags, const QModelIndex &rootIndex)
{
    if (!data_) {
        qDebug() << "DM::clearChecksums | NO data_";
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
        qDebug() << "DM::clearLostFiles | NO data_";
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
        //updateNumbers();
        moveNumbers(FileStatus::Missing, FileStatus::Removed); // experimental
    }

    return number;
}

bool DataMaintainer::itemFileRemoveLost(const QModelIndex &fileIndex)
{
    if (data_ && TreeModel::hasStatus(FileStatus::Missing, fileIndex)) {
        clearChecksum(fileIndex);
        setFileStatus(fileIndex, FileStatus::Removed);
        return true;
    }

    return false;
}

bool DataMaintainer::tryMoved(const QModelIndex &_file, const QString &_checksum)
{
    if (!data_ || !data_->_cacheMissing.contains(_checksum))
        return false;

    // moved out
    QModelIndex _i_movedout = data_->_cacheMissing.take(_checksum);
    const FileStatus _status = TreeModel::itemFileStatus(_i_movedout);

    if (_status & (FileStatus::Missing | FileStatus::Removed)) {
        clearChecksum(_i_movedout);
        setFileStatus(_i_movedout, FileStatus::MovedOut);
        data_->numbers_.moveFile(_status, FileStatus::MovedOut);

        // moved
        setFileStatus(_file, FileStatus::Moved);
        setItemValue(_file, Column::ColumnChecksum, _checksum);
        return true;
    }

    return false;
}

int DataMaintainer::updateMismatchedChecksums()
{
    if (!data_) {
        qDebug() << "DM::updateMismatchedChecksums | NO data_";
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
        const QString _reChecksum = TreeModel::itemFileReChecksum(fileIndex);

        if (!_reChecksum.isEmpty()) {
            setItemValue(fileIndex, Column::ColumnChecksum, _reChecksum);
            setItemValue(fileIndex, Column::ColumnReChecksum);
            setItemValue(fileIndex, Column::ColumnStatus, FileStatus::Updated);
            return true;
        }
    }

    return false;
}

void DataMaintainer::rollBackStoppedCalc(const QModelIndex &rootIndex, FileStatus prevStatus)
{
    if (prevStatus != FileStatus::Queued) { // data_ && !data_->isInCreation()
        if (prevStatus == FileStatus::New)
            clearChecksums(FileStatus::Added, rootIndex);
        else if (tools::isFlagCombined(prevStatus)) // CombNotChecked == NotChecked | NotCheckedMod
            prevStatus = FileStatus::NotChecked;

        changeFilesStatus((FileStatus::CombProcessing | FileStatus::Added), prevStatus, rootIndex);
    }
}

bool DataMaintainer::importJson(const QString &jsonFilePath)
{
    const bool _success = setSourceData(json_->parseJson(jsonFilePath));
    if (!_success)
        emit failedDataCreation();

    return _success;
}

void DataMaintainer::exportToJson()
{
    if (!data_)
        return;

    data_->makeBackup();

    const QString _dbFilePath = json_->makeJson(data_);

    if (!_dbFilePath.isEmpty()) {
        setDbFileState(data_->isInCreation() ? DbFileState::Created : DbFileState::Saved);
        data_->metaData_.dbFilePath = _dbFilePath;
    }
    else {
        setDbFileState(DbFileState::NotSaved);
    }

    // debug info
    if (data_->isDbFileState(DbFileState::Saved))
        qDebug() << "DM::exportToJson >> Saved";
}

void DataMaintainer::forkJsonDb(const QModelIndex &rootFolder)
{
    if (!data_)
        return;

    if (!TreeModel::isFolderRow(rootFolder)) {
        qDebug() << "DM::forkJsonDb | wrong folder index";
        return;
    }

    emit subDbForked(json_->makeJson(data_, rootFolder));
}

int DataMaintainer::importBranch(const QModelIndex &rootFolder)
{
    if (!data_)
        return 0;

    const QString _filePath = data_->branchExistFilePath(rootFolder);
    const QJsonArray _j_array = json_->loadJsonDB(_filePath);
    const QJsonObject _mainList = (_j_array.size() >= 2) ? _j_array.at(1).toObject() : QJsonObject();

    if (_mainList.isEmpty() || !data_
        || tools::algoStrLen(data_->metaData_.algorithm) != _mainList.begin().value().toString().length())
    {
        return 0;
    }

    int _num = 0;
    TreeModelIterator _it(data_->model_, rootFolder);

    while (_it.hasNext()) {
        _it.nextFile();
        if (_it.status() == FileStatus::New) {
            const QJsonValue __v = _mainList.value(_it.path(rootFolder));

            if (!__v.isUndefined()
                && importChecksum(_it.index(), __v.toString()))
            {
                ++_num;
            }
        }
    }

    if (_num) {
        setDbFileState(DbFileState::NotSaved);
        updateDateTime();
        updateNumbers();
    }

    return _num;
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
        const Numbers &_num = data_->getNumbers(curIndex);
        QStringList _sl;

        const NumSize _n_avail = _num.values(FileStatus::CombAvailable);
        if (_n_avail) {
            _sl << QStringLiteral(u"Avail.: ") + format::filesNumSize(_n_avail);
        }

        const NumSize _n_new = _num.values(FileStatus::New);
        if (_n_new) {
            _sl << QStringLiteral(u"New: ") + format::filesNumSize(_n_new);
        }

        const NumSize _n_missing = _num.values(FileStatus::Missing);
        if (_n_missing) {
            _sl << tools::joinStrings(QStringLiteral(u"Missing:"), _n_missing._num);
        }

        const NumSize _n_unr = _num.values(FileStatus::CombUnreadable);
        if (_n_unr) {
            _sl << tools::joinStrings(QStringLiteral(u"Unread.:"), _n_unr._num);
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
