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
#include "pathstr.h"

Manager::Manager(Settings *settings, QObject *parent)
    : QObject(parent), p_settings(settings)
{
    p_files->setProcState(procState);
    dataMaintainer->setProcState(procState);
    shaCalc.setProcState(procState);

    connect(&shaCalc, &ShaCalculator::doneChunk, procState, &ProcState::addChunk);
    connect(dataMaintainer, &DataMaintainer::showMessage, this, &Manager::showMessage);
    connect(dataMaintainer, &DataMaintainer::setStatusbarText, this, &Manager::setStatusbarText);
    connect(p_files, &Files::setStatusbarText, this, &Manager::setStatusbarText);

    connect(this, &Manager::taskAdded, this, &Manager::runTasks);
}

void Manager::queueTask(Task task)
{
    m_taskQueue.append(task);

    if (procState->isState(State::Idle) && m_taskQueue.size() == 1)
        emit taskAdded();
}

void Manager::runTasks()
{
    // qDebug() << thread()->objectName() << Q_FUNC_INFO << m_taskQueue.size();

    while (!m_taskQueue.isEmpty()) {
        Task task = m_taskQueue.takeFirst();
        procState->setState(task._state);
        task._func();
    }

    procState->setState(State::Idle);
}

void Manager::clearTasks()
{
    if (m_taskQueue.size() > 0)
        qDebug() << "Manager::clearTasks:" << m_taskQueue.size();

    m_taskQueue.clear();
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
    if (!dataMaintainer->folderBasedData(FileStatus::Queued)) {
        return;
    }

    emit setViewData(dataMaintainer->m_data);

    // calculating checksums
    calculateChecksums(FileStatus::Queued);

    // saving to json
    if (!procState->isCanceled()) {
        dataMaintainer->updateDateTime();
        if (dataMaintainer->exportToJson()) // if saved successfully
            sendDbUpdated();
    }
}

void Manager::processFileSha(const QString &filePath, QCryptographicHash::Algorithm algo, DestFileProc result)
{
    const QString _dig = hashFile(filePath, algo);

    if (_dig.isEmpty()) {
        calcFailedMessage(filePath);
        return;
    }

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
    fileVal.checksum = _dig.toLower();

    emit fileProcessed(filePath, fileVal);
}

void Manager::restoreDatabase()
{
    if (dataMaintainer->m_data
        && (dataMaintainer->m_data->restoreBackupFile() || dataMaintainer->isDataNotSaved()))
    {
        createDataModel(dataMaintainer->m_data->m_metadata.dbFilePath);
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

    dataMaintainer->setConsiderDateModified(p_settings->considerDateModified);

    if (dataMaintainer->importJson(dbFilePath)) {
        if (p_settings->detectMoved)
            cacheMissingItems();

        emit setViewData(dataMaintainer->m_data);
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
    else
        emit switchToFsPrepared();

    qDebug() << "Manager::prepareSwitchToFs >> Done";
}

void Manager::updateDatabase(const DbMod dest)
{
    if (!dataMaintainer->m_data || dataMaintainer->m_data->isImmutable())
        return;

    const Numbers &nums = dataMaintainer->m_data->m_numbers;

    if (!nums.contains(FileStatus::CombAvailable)) {
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

        if (nums.numberOf(FileStatus::MovedOut) > 0)
            dataMaintainer->setDbFileState(DbFileState::NotSaved);
        else
            emit showMessage("No Moved items found");
    }
    else {
        if ((dest & DM_AddNew)
            && nums.contains(FileStatus::New))
        {
            const int numAdded = calculateChecksums(FileStatus::New);

            if (procState->isCanceled())
                return;
            else if (numAdded > 0)
                dataMaintainer->setDbFileState(DbFileState::NotSaved);
        }

        if ((dest & DM_ClearLost)
            && nums.contains(FileStatus::Missing))
        {
            dataMaintainer->clearLostFiles();
        }
    }

    if (dataMaintainer->isDataNotSaved()) {
        dataMaintainer->updateDateTime();

        if (p_settings->instantSaving)
            dataMaintainer->saveData();            

        sendDbUpdated();
    }
}

void Manager::updateItemFile(const QModelIndex &fileIndex, DbMod job)
{
    const DataContainer *_data = dataMaintainer->m_data;
    const FileStatus _prevStatus = TreeModel::itemFileStatus(fileIndex);

    if (!_data
        || _data->isImmutable()
        || !(_prevStatus & FileStatus::CombUpdatable))
    {
        return;
    }

    if (_prevStatus == FileStatus::New) {
        if (job & (DM_ImportDigest | DM_PasteDigest)) {
            const QString __d = (job == DM_ImportDigest) ? extractDigestFromFile(_data->digestFilePath(fileIndex))
                                                          : TreeModel::itemFileChecksum(fileIndex);

            if (tools::canBeChecksum(__d, _data->m_metadata.algorithm)) // checking for compliance with the current algo
                dataMaintainer->importChecksum(fileIndex, __d);
        }
        else { // calc the new one
            const QString _dig = hashItem(fileIndex);

            if (_dig.isEmpty()) // return previous status
                dataMaintainer->setFileStatus(fileIndex, _prevStatus);
            else
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

        if (p_settings->instantSaving) {
            saveData();
        }
        else { // temp solution to update Button info
            emit procState->stateChanged();
        }
    }
}

void Manager::importBranch(const QModelIndex &rootFolder)
{
    const int imported = dataMaintainer->importBranch(rootFolder);

    if (imported > 0 && p_settings->instantSaving) {
        dataMaintainer->saveData();
    }

    if (imported == -1) {
        emit showMessage("Only checksums of the same Algorithm can be imported.",
                         "Inappropriate Algorithm"); // title
    } else {
        emit showMessage(QString::number(imported) + " checksums imported");
    }
}

void Manager::branchSubfolder(const QModelIndex &subfolder)
{
    dataMaintainer->forkJsonDb(subfolder);
}

// check only selected file instead of full database verification
void Manager::verifyFileItem(const QModelIndex &fileItemIndex)
{
    if (!dataMaintainer->m_data) {
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

    const QString _sum = hashItem(fileItemIndex, Verification);

    if (!_sum.isEmpty()) {
        showFileCheckResultMessage(dataMaintainer->m_data->itemAbsolutePath(fileItemIndex), storedSum, _sum);
        dataMaintainer->updateChecksum(fileItemIndex, _sum);
        dataMaintainer->updateNumbers(fileItemIndex, storedStatus);
    } else if (procState->isCanceled()) {
        // return previous status
        dataMaintainer->setFileStatus(fileItemIndex, storedStatus);
    }
}

void Manager::verifyFolderItem(const QModelIndex &folderItemIndex, FileStatus checkstatus)
{
    if (!dataMaintainer->m_data) {
        return;
    }

    if (!dataMaintainer->m_data->contains(FileStatus::CombAvailable, folderItemIndex)) {
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
    if (dataMaintainer->m_data->m_numbers.contains(flagAddedUpdated)) {
        dataMaintainer->changeFilesStatus(flagAddedUpdated, FileStatus::Matched, folderItemIndex);
    }

    // result
    if (!folderItemIndex.isValid()) { // if root folder
        emit folderChecked(dataMaintainer->m_data->m_numbers);

        // Save the verification datetime, if needed
        if (p_settings->saveVerificationDateTime) {
            dataMaintainer->updateVerifDateTime();
        }
    }
    else { // if subfolder
        const Numbers &nums = dataMaintainer->m_data->getNumbers(folderItemIndex);
        const QString subfolderName = TreeModel::itemName(folderItemIndex);

        emit folderChecked(nums, subfolderName);
    }
}

void Manager::checkSummaryFile(const QString &path)
{
    const QString storedChecksum = extractDigestFromFile(path);

    if (storedChecksum.isEmpty())
        return;

    const QString checkFileName = QFileInfo(path).completeBaseName();
    QString checkFilePath = pathstr::joinPath(pathstr::parentFolder(path), checkFileName);

    if (!QFileInfo(checkFilePath).isFile()) {
        QFile sumFile(path);
        if (!sumFile.open(QFile::ReadOnly))
            return;

        const QString line = sumFile.readLine();
        checkFilePath = pathstr::joinPath(pathstr::parentFolder(path), line.mid(storedChecksum.length() + 2).remove('\n'));

        if (!QFileInfo(checkFilePath).isFile()) {
            emit showMessage("No File to check", "Warning");
            return;
        }
    }

    checkFile(checkFilePath, storedChecksum);
}

QString Manager::extractDigestFromFile(const QString &digest_file)
{
    QFile sumFile(digest_file);
    if (!sumFile.open(QFile::ReadOnly)) {
        emit showMessage("Error while reading Summary File", "Error");
        return QString();
    }

    const QString line = sumFile.readLine();

    static const QStringList l_seps { "  ", " *" };

    for (int it = 0; it <= l_seps.size(); ++it) {
        int cut = (it < l_seps.size()) ? line.indexOf(l_seps.at(it))
                                       : tools::algoStrLen(tools::strToAlgo(pathstr::suffix(digest_file)));

        if (cut > 0) {
            const QString dig = line.left(cut);
            if (tools::canBeChecksum(dig)) {
                return dig;
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
    const QString _digest = hashFile(filePath, algo, Verification);

    if (!_digest.isEmpty()) {
        showFileCheckResultMessage(filePath, checkSum, _digest);
    } else {
        calcFailedMessage(filePath);
    }
}

void Manager::calcFailedMessage(const QString &filePath)
{
    if (!procState->isCanceled()) {
        QString __s = tools::enumToString(tools::failedCalcStatus(filePath)) + ":\n";

        emit showMessage(__s += filePath, "Warning");
        emit setStatusbarText("failed to read file");
    }
}

QString Manager::hashFile(const QString &filePath, QCryptographicHash::Algorithm algo, const CalcKind calckind)
{
    if (!procState->hasTotalSize())
        procState->setTotalSize(QFileInfo(filePath).size());

    updateProgText(calckind, filePath);

    return shaCalc.calculate(filePath, algo);
}

QString Manager::hashItem(const QModelIndex &ind, const CalcKind calckind)
{
    dataMaintainer->setFileStatus(ind,
                                  calckind ? FileStatus::Verifying : FileStatus::Calculating);

    const QString filePath = dataMaintainer->m_data->itemAbsolutePath(ind);
    const QString digest = hashFile(filePath, dataMaintainer->m_data->m_metadata.algorithm, calckind);

    if (digest.isEmpty() && !procState->isCanceled()) {
        dataMaintainer->setFileStatus(ind, tools::failedCalcStatus(filePath, calckind));
    }

    return digest;
}

void Manager::updateProgText(const CalcKind calckind, const QString &file)
{
    const QString _purp = calckind ? QStringLiteral(u"Verifying") : QStringLiteral(u"Calculating");
    const Chunks<qint64> _p_size = procState->pSize();
    const Chunks<int> _p_queue = procState->pQueue();

    // single file
    if (!_p_queue.hasSet()) {
        // _purp: file (size)
        const QString __s = tools::joinStrings(_purp,
                                               format::fileNameAndSize(file, _p_size._total),
                                               Lit::s_sepColonSpace);
        emit setStatusbarText(__s);
        return;
    }

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

int Manager::calculateChecksums(const FileStatus status, const QModelIndex &root)
{
    return calculateChecksums(DM_AutoSelect, status, root);
}

int Manager::calculateChecksums(const DbMod purpose, const FileStatus status, const QModelIndex &root)
{
    DataContainer *_data = dataMaintainer->m_data;

    if (!_data
        || (root.isValid() && root.model() != _data->m_model))
    {
        qDebug() << "Manager::calculateChecksums | No data or wrong rootIndex";
        return 0;
    }

    if (status != FileStatus::Queued)
        dataMaintainer->addToQueue(status, root);

    const NumSize _queued = _data->getNumbers(root).values(FileStatus::Queued);

    qDebug() << "Manager::calculateChecksums | Queued:" << _queued._num;

    if (!_queued)
        return 0;

    procState->setTotal(_queued);

    bool isMismatchFound = false;

    // checking whether this is a Calculation or Verification process
    const CalcKind _calckind = (status & FileStatus::CombAvailable) ? Verification : Calculation;

    // process
    TreeModelIterator iter(_data->m_model, root);

    while (iter.hasNext() && !procState->isCanceled()) {
        if (iter.nextFile().status() != FileStatus::Queued)
            continue;

        const QString _checksum = hashItem(iter.index(), _calckind); // hashing

        if (procState->isCanceled())
            break;

        if (_checksum.isEmpty()) {
            procState->decreaseTotalQueued();
            procState->decreaseTotalSize(iter.size());
            continue;
        }

        // success
        procState->addDoneOne();

        if (purpose == DM_FindMoved) {
            if (!dataMaintainer->tryMoved(iter.index(), _checksum))
                dataMaintainer->setFileStatus(iter.index(), status); // rollback status
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
        dataMaintainer->rollBackStoppedCalc(root, status);
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
    if (m_isViewFileSysytem) {
        QFileInfo fileInfo(path);

        if (fileInfo.isFile()) {
            emit setStatusbarText(format::fileNameAndSize(path));
        }
        else if (fileInfo.isDir()) {
            emit setStatusbarText(QStringLiteral(u"counting..."));
            emit setStatusbarText(p_files->getFolderSize(path));
        }
    }
}

void Manager::getIndexInfo(const QModelIndex &curIndex)
{
    if (!m_isViewFileSysytem && dataMaintainer->m_data)
        emit setStatusbarText(dataMaintainer->itemContentsInfo(curIndex));
}

void Manager::folderContentsList(const QString &folderPath, bool filterCreation)
{
    if (m_isViewFileSysytem) {
        if (Files::isEmptyFolder(folderPath)) {
            emit showMessage("There are no file types to display.", "Empty folder");
            return;
        }

        FilterRule _comb_attr(FilterAttribute::NoAttributes);
        if (p_settings->filter_ignore_unpermitted)
            _comb_attr.addAttribute(FilterAttribute::IgnoreUnpermitted);
        if (p_settings->filter_ignore_symlinks)
            _comb_attr.addAttribute(FilterAttribute::IgnoreSymlinks);

        const FileTypeList _typesList = p_files->getFileTypes(folderPath, _comb_attr);

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
    if (!dataMaintainer->m_data)
        return;

    const FileTypeList _typesList = p_files->getFileTypes(dataMaintainer->m_data->m_model);

    if (!_typesList.isEmpty())
        emit dbContentsListCreated(dataMaintainer->m_data->m_metadata.workDir, _typesList);
}

void Manager::cacheMissingItems()
{
    DataContainer *_data = dataMaintainer->m_data;

    if (!_data || !_data->hasPossiblyMovedItems()) {
        return;
    }

    TreeModelIterator it(_data->m_model);

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
    m_isViewFileSysytem = (modelView == ModelView::FileSystem);
}
