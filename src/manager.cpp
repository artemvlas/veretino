/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "manager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QThread>
#include <QTimer>
#include <QDebug>
#include <QStringBuilder>
#include "files.h"
#include "treemodeliterator.h"
#include "tools.h"

Manager::Manager(Settings *settings, QObject *parent)
    : QObject(parent), settings_(settings)
{
    files_->setProcState(procState);
    dataMaintainer->setProcState(procState);
    shaCalc.setProcState(procState);

    connect(&shaCalc, &ShaCalculator::doneChunk, procState, &ProcState::addChunk);
    connect(dataMaintainer, &DataMaintainer::showMessage, this, &Manager::showMessage);
    connect(dataMaintainer, &DataMaintainer::setStatusbarText, this, &Manager::setStatusbarText);
    connect(files_, &Files::setStatusbarText, this, &Manager::setStatusbarText);

    connect(this, &Manager::taskAdded, this, &Manager::runTasks);
}

void Manager::queueTask(Task task)
{
    taskQueue_.append(task);

    if (procState->isState(State::Idle) && taskQueue_.size() == 1)
        emit taskAdded();
}

void Manager::runTasks()
{
    // qDebug() << thread()->objectName() << Q_FUNC_INFO << taskQueue_.size();

    while (!taskQueue_.isEmpty()) {
        Task _task = taskQueue_.takeFirst();
        procState->setState(_task._state);
        _task._func();
    }

    procState->setState(State::Idle);
}

void Manager::clearTasks()
{
    if (taskQueue_.size() > 0)
        qDebug() << "Manager::clearTasks:" << taskQueue_.size();

    taskQueue_.clear();
}

void Manager::sendDbUpdated()
{
    QTimer::singleShot(0, dataMaintainer, &DataMaintainer::databaseUpdated);
}

void Manager::processFolderSha(const MetaData &metaData)
{
    if (Files::isEmptyFolder(metaData.workDir, metaData.filter)) {
        emit showMessage("All files have been excluded.\n"
                         "Filtering rules can be changed in the settings.", "No proper files");
        return;
    }

    dataMaintainer->setSourceData(metaData);

    // create the filelist
    if (!dataMaintainer->folderBasedData(FileStatus::Queued,
                                         settings_->excludeUnpermitted))
    {
        return;
    }

    emit setViewData(dataMaintainer->data_);

    // calculating checksums
    calculateChecksums(FileStatus::Queued);

    if (!procState->isCanceled()) { // saving to json
        dataMaintainer->updateDateTime();
        dataMaintainer->exportToJson();
        sendDbUpdated();
    }
}

void Manager::processFileSha(const QString &filePath, QCryptographicHash::Algorithm algo, DestFileProc result)
{
    QString sum = calculateChecksum(filePath, algo);

    if (sum.isEmpty())
        return;

    FileStatus purpose;

    switch (result) {
        case SumFile:
            purpose = FileStatus::ToSumFile;
            break;
        case Clipboard:
            purpose = FileStatus::ToClipboard;
            break;
        default:
            purpose = FileStatus::Computed;
            break;
    }

    FileValues fileVal(purpose, QFileInfo(filePath).size());
    fileVal.checksum = sum.toLower();

    emit fileProcessed(filePath, fileVal);
}

void Manager::restoreDatabase()
{
    if (dataMaintainer->data_
        && (dataMaintainer->data_->restoreBackupFile() || dataMaintainer->isDataNotSaved()))
    {
        createDataModel(dataMaintainer->data_->metaData_.dbFilePath);
    }
    else {
        emit setStatusbarText("No saved changes");
    }
}

void Manager::createDataModel(const QString &dbFilePath)
{
    if (!paths::isDbFile(dbFilePath)) {
        qDebug() << "Manager::createDataModel | Wrong DB file:" << dbFilePath;
        return;
    }

    dataMaintainer->setConsiderDateModified(settings_->considerDateModified);

    if (dataMaintainer->importJson(dbFilePath)) {
        if (settings_->detectMoved)
            cacheMissingItems();

        emit setViewData(dataMaintainer->data_);
        sendDbUpdated(); // using timer 0
    }
}

void Manager::saveData()
{
    if (dataMaintainer->isDataNotSaved()) {
        dataMaintainer->saveData();
    }
}

void Manager::prepareSwitchToFs()
{
    saveData();

    if (dataMaintainer->isDataNotSaved())
        emit showMessage("The Database is NOT saved", "Error");

    emit switchToFsPrepared();

    qDebug() << "Manager::prepareSwitchToFs >> Done";
}

void Manager::updateDatabase(const DbMod dest)
{
    if (!dataMaintainer->data_ || dataMaintainer->data_->isImmutable())
        return;

    const Numbers &_num = dataMaintainer->data_->numbers_;

    if (!_num.contains(FileStatus::CombAvailable)) {
        emit showMessage("Failure to delete all database items.\n\n" + movedDbWarning, "Warning");
        return;
    }

    if (dest == DM_UpdateMismatches) {
        dataMaintainer->updateMismatchedChecksums();
    }
    else if (dest == DM_FindMoved) {
        calculateChecksums(DM_FindMoved, FileStatus::New);

        if (procState->isCanceled())
            return;

        if (_num.numberOf(FileStatus::MovedOut) > 0)
            dataMaintainer->setDbFileState(DbFileState::NotSaved);
        else
            emit showMessage("No Moved items found");
    }
    else {
        if ((dest & DM_AddNew)
            && _num.contains(FileStatus::New))
        {
            const int numAdded = calculateChecksums(FileStatus::New);

            if (procState->isCanceled())
                return;
            else if (numAdded > 0)
                dataMaintainer->setDbFileState(DbFileState::NotSaved);
        }

        if ((dest & DM_ClearLost)
            && _num.contains(FileStatus::Missing))
        {
            dataMaintainer->clearLostFiles();
        }
    }

    if (dataMaintainer->isDataNotSaved()) {
        dataMaintainer->updateDateTime();

        if (settings_->instantSaving)
            dataMaintainer->saveData();            

        sendDbUpdated();
    }
}

void Manager::updateItemFile(const QModelIndex &fileIndex, DbMod _job)
{
    const DataContainer *_data = dataMaintainer->data_;
    const FileStatus _prevStatus = TreeModel::itemFileStatus(fileIndex);

    if (!_data
        || _data->isImmutable()
        || !(_prevStatus & FileStatus::CombUpdatable))
    {
        return;
    }

    if (_prevStatus == FileStatus::New) {
        QString _dig;

        if (_job == DM_ImportDigest) {
            const QString __d = extractDigestFromFile(_data->digestFilePath(fileIndex));
            if (tools::canBeChecksum(__d, _data->metaData_.algorithm)) // checking for compliance with the current algo
                _dig = __d;
        }
        else { // calc the new one
            dataMaintainer->setItemValue(fileIndex, Column::ColumnStatus, FileStatus::Calculating);
            _dig = calculateChecksum(_data->itemAbsolutePath(fileIndex),
                                     _data->metaData_.algorithm);

            if (_dig.isEmpty()) // return previous status
                dataMaintainer->setItemValue(fileIndex, Column::ColumnStatus, _prevStatus);
        }

        if (!_dig.isEmpty()) {
            dataMaintainer->updateChecksum(fileIndex, _dig);
        }
    }
    else if (_prevStatus == FileStatus::Missing) {
        dataMaintainer->itemFileRemoveLost(fileIndex);
    }
    else if (_prevStatus == FileStatus::Mismatched) {
        dataMaintainer->itemFileUpdateChecksum(fileIndex);
    }

    if (TreeModel::hasStatus(FileStatus::CombDbChanged, fileIndex)) {
        dataMaintainer->setDbFileState(DbFileState::NotSaved);
        dataMaintainer->updateNumbers(fileIndex, _prevStatus);
        dataMaintainer->updateDateTime();

        if (settings_->instantSaving) {
            saveData();
        }
        else { // temp solution to update Button info
            emit procState->stateChanged();
        }
    }
}

void Manager::branchSubfolder(const QModelIndex &subfolder)
{
    dataMaintainer->forkJsonDb(subfolder);
}

// check only selected file instead of full database verification
void Manager::verifyFileItem(const QModelIndex &fileItemIndex)
{
    if (!dataMaintainer->data_) {
        return;
    }

    const FileStatus storedStatus = TreeModel::itemFileStatus(fileItemIndex);
    const QString storedSum = TreeModel::itemFileChecksum(fileItemIndex);

    if (!tools::canBeChecksum(storedSum) || !(storedStatus & FileStatus::CombAvailable)) {
        switch (storedStatus) {
        case FileStatus::Missing:
            emit showMessage("File does not exist", "Missing File");
            break;
        case FileStatus::Removed:
            emit showMessage("The checksum has been removed from the database.", "No Checksum");
            break;
        case FileStatus::UnPermitted:
        case FileStatus::ReadError:
            emit showMessage("The integrity of this file is not verifiable.\n"
                             "Failed to read: no access or disk error.", "Unreadable File");
            break;
        default:
            emit showMessage("No checksum in the database.", "No Checksum");
            break;
        }

        return;
    }

    dataMaintainer->setItemValue(fileItemIndex, Column::ColumnStatus, FileStatus::Verifying);
    const QString filePath = dataMaintainer->data_->itemAbsolutePath(fileItemIndex);
    const QString sum = calculateChecksum(filePath, dataMaintainer->data_->metaData_.algorithm, true);

    if (sum.isEmpty()) { // return previous status
        dataMaintainer->setItemValue(fileItemIndex, Column::ColumnStatus, storedStatus);
    }
    else {
        showFileCheckResultMessage(filePath, storedSum, sum);
        dataMaintainer->updateChecksum(fileItemIndex, sum);
        dataMaintainer->updateNumbers(fileItemIndex, storedStatus);
    }
}

void Manager::verifyFolderItem(const QModelIndex &folderItemIndex, FileStatus checkstatus)
{
    if (!dataMaintainer->data_) {
        return;
    }

    if (!dataMaintainer->data_->contains(FileStatus::CombAvailable, folderItemIndex)) {
        QString warningText = "There are no files available for verification.";
        if (!folderItemIndex.isValid())
            warningText.append("\n\n" + movedDbWarning);

        emit showMessage(warningText, "Warning");
        return;
    }

    // main job
    calculateChecksums(checkstatus, folderItemIndex);

    if (procState->isCanceled())
        return;

    // changing accompanying statuses to "Matched"
    FileStatuses flagAddedUpdated = (FileStatus::Added | FileStatus::Updated);
    if (dataMaintainer->data_->numbers_.contains(flagAddedUpdated)) {
        dataMaintainer->changeFilesStatus(flagAddedUpdated, FileStatus::Matched, folderItemIndex);
    }

    // result
    if (!folderItemIndex.isValid()) { // if root folder
        emit folderChecked(dataMaintainer->data_->numbers_);

        // Save the verification datetime, if needed
        if (settings_->saveVerificationDateTime) {
            dataMaintainer->updateVerifDateTime();
        }
    }
    else { // if subfolder
        const Numbers &num = dataMaintainer->data_->getNumbers(folderItemIndex);
        QString subfolderName = TreeModel::itemName(folderItemIndex);

        emit folderChecked(num, subfolderName);
    }
}

void Manager::checkSummaryFile(const QString &path)
{
    const QString storedChecksum = extractDigestFromFile(path);

    if (storedChecksum.isEmpty())
        return;

    const QString checkFileName = QFileInfo(path).completeBaseName();
    QString checkFilePath = paths::joinPath(paths::parentFolder(path), checkFileName);

    if (!QFileInfo(checkFilePath).isFile()) {
        QFile sumFile(path);
        if (!sumFile.open(QFile::ReadOnly))
            return;

        const QString line = sumFile.readLine();
        checkFilePath = paths::joinPath(paths::parentFolder(path), line.mid(storedChecksum.length() + 2).remove('\n'));

        if (!QFileInfo(checkFilePath).isFile()) {
            emit showMessage("No File to check", "Warning");
            return;
        }
    }

    checkFile(checkFilePath, storedChecksum);
}

QString Manager::extractDigestFromFile(const QString &_digest_file)
{
    QFile sumFile(_digest_file);
    if (!sumFile.open(QFile::ReadOnly)) {
        emit showMessage("Error while reading Summary File", "Error");
        return QString();
    }

    const QString line = sumFile.readLine();

    static const QStringList _l_seps { "  ", " *" };

    for (int it = 0; it <= _l_seps.size(); ++it) {
        int _cut = (it < _l_seps.size()) ? line.indexOf(_l_seps.at(it))
                                         : tools::algoStrLen(tools::strToAlgo(paths::suffix(_digest_file)));

        if (_cut > 0) {
            const QString _dig = line.left(_cut);
            if (tools::canBeChecksum(_dig)) {
                return _dig;
            }
        }
    }

    emit showMessage("Checksum not found", "Warning");
    return QString();
}

void Manager::checkFile(const QString &filePath, const QString &checkSum)
{
    checkFile(filePath, checkSum, tools::algoByStrLen(checkSum.length()));
}

void Manager::checkFile(const QString &filePath, const QString &checkSum, QCryptographicHash::Algorithm algo)
{
    const QString _digest = calculateChecksum(filePath, algo, true);

    if (!_digest.isEmpty()) {
        showFileCheckResultMessage(filePath, checkSum, _digest);
    }
}

QString Manager::calculateChecksum(const QString &filePath, QCryptographicHash::Algorithm algo, const bool _isVerif)
{
    procState->setTotalSize(QFileInfo(filePath).size());

    const QString _calcPurp = _isVerif ? QStringLiteral(u"Verifying") : QStringLiteral(u"Calculating");
    emit setStatusbarText(QString("%1 %2: %3").arg(_calcPurp,
                                                   format::algoToStr(algo),
                                                   format::fileNameAndSize(filePath)));

    const QString _digest = shaCalc.calculate(filePath, algo);

    if (_digest.isEmpty() && !procState->isCanceled()) {
        QString __s = tools::enumToString(tools::failedCalcStatus(filePath)) + ":\n";

        emit showMessage(__s += filePath, "Warning");
        emit setStatusbarText("failed to read file");
    }

    return _digest;
}

QString Manager::hashItem(const QModelIndex &_ind, const bool _isVerif)
{
    dataMaintainer->setFileStatus(_ind,
                                  _isVerif ? FileStatus::Verifying : FileStatus::Calculating);

    const QString _filePath = dataMaintainer->data_->itemAbsolutePath(_ind);
    const QString _checksum = shaCalc.calculate(_filePath, dataMaintainer->data_->metaData_.algorithm);

    if (_checksum.isEmpty() && !procState->isCanceled()) {
        dataMaintainer->setFileStatus(_ind, tools::failedCalcStatus(_filePath, _isVerif));
    }

    return _checksum;
}

void Manager::updateProgText(const bool _isVerif)
{
    const QString _purp = _isVerif ? QStringLiteral(u"Verifying") : QStringLiteral(u"Calculating");
    const Chunks<qint64> _p_size = procState->pSize();
    const Chunks<int> _p_queue = procState->pQueue();

    // to avoid calling the dataSizeReadable() func for each file
    static qint64 _lastTotalSize;
    static QString _lastTotalSizeR;

    if (!_p_size.hasChunks() || (_lastTotalSize != _p_size._total)) {
        _lastTotalSize = _p_size._total;
        _lastTotalSizeR = format::dataSizeReadable(_lastTotalSize);
    }

    // UGLY, but better performance (should be). And should be re-implemented.
    const QString _res = _purp % ' ' % QString::number(_p_queue._done + 1) % QStringLiteral(u" of ")
                    % QString::number(_p_queue._total) % QStringLiteral(u" checksums ")
                    % (!_p_size.hasChunks() ? format::inParentheses(_lastTotalSizeR) // (%1)
                    : ('(' % format::dataSizeReadable(_p_size._done) % QStringLiteral(u" / ") % _lastTotalSizeR % ')')); // "(%1 / %2)"

    emit setStatusbarText(_res);

    // OLD
    /*emit setStatusbarText(QString("%1 %2 of %3 checksums %4")
                              .arg(_purp)
                              .arg(_p_items._done + 1)
                              .arg(_p_items._total)
                              .arg(doneData));*/
}

int Manager::calculateChecksums(const FileStatus _status, const QModelIndex &_root)
{
    return calculateChecksums(DM_AutoSelect, _status, _root);
}

int Manager::calculateChecksums(const DbMod _purpose, const FileStatus _status, const QModelIndex &_root)
{
    DataContainer *_data = dataMaintainer->data_;

    if (!_data
        || (_root.isValid() && _root.model() != _data->model_))
    {
        qDebug() << "Manager::calculateChecksums | No data or wrong rootIndex";
        return 0;
    }

    if (_status != FileStatus::Queued)
        dataMaintainer->addToQueue(_status, _root);

    const NumSize _queued = _data->getNumbers(_root).values(FileStatus::Queued);

    qDebug() << "Manager::calculateChecksums | Queued:" << _queued._num;

    if (!_queued)
        return 0;

    procState->setTotal(_queued);

    bool isMismatchFound = false;

    // checking whether this is a Calculation or Verification process
    const bool _isVerif = (_status & FileStatus::CombAvailable);

    // process
    TreeModelIterator iter(_data->model_, _root);

    while (iter.hasNext() && !procState->isCanceled()) {
        if (iter.nextFile().status() != FileStatus::Queued)
            continue;

        updateProgText(_isVerif); // update statusbar text
        const QString _checksum = hashItem(iter.index(), _isVerif); // hashing

        if (procState->isCanceled())
            break;

        if (_checksum.isEmpty()) {
            procState->decreaseTotalQueued();
            procState->decreaseTotalSize(iter.size());
            continue;
        }

        // success
        procState->addDoneOne();

        if (_purpose == DM_FindMoved) {
            if (!dataMaintainer->tryMoved(iter.index(), _checksum))
                dataMaintainer->setFileStatus(iter.index(), _status); // rollback status
            continue;
        }

        // != DM_FindMoved
        if (!dataMaintainer->updateChecksum(iter.index(), _checksum)
            && !isMismatchFound) // the signal is only needed once
        {
            emit mismatchFound();
            isMismatchFound = true;
        }
    }

    const int _done = procState->pQueue()._done;
    if (procState->isCanceled()) {
        if (procState->isState(State::Abort)) {
            qDebug() << "Manager::calculateChecksums >> Aborted";
            return 0;
        }

        // rolling back file statuses
        dataMaintainer->rollBackStoppedCalc(_root, _status);
        qDebug() << "Manager::calculateChecksums >> Stopped | Done" << _done;
    }

    // end
    dataMaintainer->updateNumbers();
    return _done;
}

void Manager::showFileCheckResultMessage(const QString &filePath, const QString &checksumEstimated, const QString &checksumCalculated)
{
    const int _compare = checksumEstimated.compare(checksumCalculated, Qt::CaseInsensitive);
    FileStatus status = (_compare == 0) ? FileStatus::Matched : FileStatus::Mismatched;
    FileValues fileVal(status, QFileInfo(filePath).size());
    fileVal.checksum = checksumEstimated.toLower();

    if (_compare) { // Mismatched
        fileVal.reChecksum = checksumCalculated;
    }

    emit fileProcessed(filePath, fileVal);
}

// info about folder (number of files and total size) or file (size)
void Manager::getPathInfo(const QString &path)
{
    if (isViewFileSysytem) {
        QFileInfo fileInfo(path);

        if (fileInfo.isFile()) {
            emit setStatusbarText(format::fileNameAndSize(path));
        }
        else if (fileInfo.isDir()) {
            emit setStatusbarText(QStringLiteral(u"counting..."));
            emit setStatusbarText(files_->getFolderSize(path));
        }
    }
}

void Manager::getIndexInfo(const QModelIndex &curIndex)
{
    if (!isViewFileSysytem && dataMaintainer->data_)
        emit setStatusbarText(dataMaintainer->itemContentsInfo(curIndex));
}

void Manager::folderContentsList(const QString &folderPath, bool filterCreation)
{
    if (isViewFileSysytem) {
        if (Files::isEmptyFolder(folderPath)) {
            emit showMessage("There are no file types to display.", "Empty folder");
            return;
        }

        const FileTypeList _typesList = files_->getFileTypes(folderPath, settings_->excludeUnpermitted);

        if (!_typesList.isEmpty()) {
            if (filterCreation)
                emit dbCreationDataCollected(folderPath, Files::dbFiles(folderPath), _typesList);
            else
                emit folderContentsListCreated(folderPath, _typesList);
        }
    }
}

void Manager::makeDbContentsList()
{
    if (!dataMaintainer->data_)
        return;

    const FileTypeList _typesList = files_->getFileTypes(dataMaintainer->data_->model_);

    if (!_typesList.isEmpty())
        emit dbContentsListCreated(dataMaintainer->data_->metaData_.workDir, _typesList);
}

void Manager::cacheMissingItems()
{
    DataContainer *_data = dataMaintainer->data_;

    if (!_data || !_data->hasPossiblyMovedItems()) {
        return;
    }

    TreeModelIterator it(_data->model_);

    while (it.hasNext()) {
        if (it.nextFile().status() == FileStatus::Missing) {
            const QString _chsum = it.checksum();
            if (!_data->_cacheMissing.contains(_chsum))
                _data->_cacheMissing[_chsum] = it.index();
        }
    }
}

void Manager::modelChanged(ModelView modelView)
{
    isViewFileSysytem = (modelView == ModelView::FileSystem);
}
