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
    : QObject(parent), m_settings(settings)
{
    m_files->setProcState(m_proc);
    m_dataMaintainer->setProcState(m_proc);
    m_shaCalc.setProcState(m_proc);

    connect(&m_shaCalc, &Hasher::doneChunk, m_proc, &ProcState::addChunk);
    connect(m_dataMaintainer, &DataMaintainer::showMessage, this, &Manager::showMessage);
    connect(m_dataMaintainer, &DataMaintainer::setStatusbarText, this, &Manager::setStatusbarText);
    connect(m_files, &Files::setStatusbarText, this, &Manager::setStatusbarText);

    connect(this, &Manager::taskAdded, this, &Manager::runTasks);
}

void Manager::queueTask(Task task)
{
    m_taskQueue.append(task);

    if (m_proc->isState(State::Idle) && m_taskQueue.size() == 1)
        emit taskAdded();
}

void Manager::runTasks()
{
    // qDebug() << thread()->objectName() << Q_FUNC_INFO << m_taskQueue.size();

    while (!m_taskQueue.isEmpty()) {
        Task task = m_taskQueue.takeFirst();
        m_proc->setState(task.state);
        task.job();
    }

    m_proc->setState(State::Idle);
}

void Manager::clearTasks()
{
    if (m_taskQueue.size() > 0)
        qDebug() << "Manager::clearTasks:" << m_taskQueue.size();

    m_taskQueue.clear();
}

void Manager::sendDbUpdated()
{
    QTimer::singleShot(0, m_dataMaintainer, &DataMaintainer::databaseUpdated);
}

void Manager::processFolderSha(const MetaData &metaData)
{
    if (Files::isEmptyFolder(metaData.workDir, metaData.filter)) {
        emit showMessage("All files have been excluded.\n"
                         "Filtering rules can be changed in the settings.", "No proper files");
        return;
    }

    // create the filelist
    if (!m_dataMaintainer->setFolderBasedData(metaData, FileStatus::Queued)) {
        return;
    }

    emit setViewData(m_dataMaintainer->m_data);

    // calculating checksums
    calculateChecksums(FileStatus::Queued);

    // saving to json
    if (!m_proc->isCanceled()) {
        m_dataMaintainer->updateDateTime();
        if (m_dataMaintainer->exportToJson()) // if saved successfully
            sendDbUpdated();
    }
}

void Manager::processFileSha(const QString &filePath, QCryptographicHash::Algorithm algo, FileValues::HashingPurpose purp)
{
    // hashing
    FileValues fileVal = hashFile(filePath, algo);

    // result handling
    if (!m_proc->isCanceled()) {
        fileVal.hash_purpose = purp;
        emit fileProcessed(filePath, fileVal);
    }
}

void Manager::restoreDatabase()
{
    if (m_dataMaintainer->m_data
        && (DataHelper::restoreBackupFile(m_dataMaintainer->m_data) || m_dataMaintainer->isDataNotSaved()))
    {
        createDataModel(m_dataMaintainer->m_data->m_metadata.dbFilePath);
    } else {
        emit setStatusbarText("No saved changes");
    }
}

void Manager::createDataModel(const QString &dbFilePath)
{
    if (!paths::isDbFile(dbFilePath)) {
        qWarning() << "Manager::createDataModel | Wrong DB file:" << dbFilePath;
        return;
    }

    m_dataMaintainer->setConsiderDateModified(m_settings->considerDateModified);

    if (m_dataMaintainer->importJson(dbFilePath)) {
        if (m_settings->detectMoved)
            cacheMissingItems();

        emit setViewData(m_dataMaintainer->m_data);
        sendDbUpdated(); // using timer 0
    }
}

void Manager::saveData()
{
    if (m_dataMaintainer->isDataNotSaved()) {
        m_dataMaintainer->saveData();
    }
}

void Manager::prepareSwitchToFs()
{
    saveData();

    if (m_dataMaintainer->isDataNotSaved()) {
        qWarning() << "Warning! The Database is NOT saved";
    } else {
        emit switchToFsPrepared();
        qDebug() << "Manager::prepareSwitchToFs >> Done";
    }
}

void Manager::updateDatabase(const DbMod dest)
{
    if (!m_dataMaintainer->m_data || DataHelper::isImmutable(m_dataMaintainer->m_data))
        return;

    const Numbers &nums = m_dataMaintainer->m_data->m_numbers;

    if (!nums.contains(FileStatus::CombAvailable)) {
        emit showMessage("Failure to delete all database items.\n\n" + k_movedDbWarning, "Warning");
        return;
    }

    if (dest == DM_UpdateMismatches) {
        m_dataMaintainer->updateMismatchedChecksums();
    } else if (dest == DM_FindMoved) {
        calculateChecksums(DM_FindMoved, FileStatus::New);

        if (m_proc->isCanceled())
            return;

        if (nums.numberOf(FileStatus::MovedOut) > 0)
            m_dataMaintainer->setDbFileState(DbFileState::NotSaved);
        else
            emit showMessage("No Moved items found");
    } else {
        if ((dest & DM_AddNew)
            && nums.contains(FileStatus::New))
        {
            const int numAdded = calculateChecksums(FileStatus::New);

            if (m_proc->isCanceled())
                return;
            else if (numAdded > 0)
                m_dataMaintainer->setDbFileState(DbFileState::NotSaved);
        }

        if ((dest & DM_ClearLost)
            && nums.contains(FileStatus::Missing))
        {
            m_dataMaintainer->clearLostFiles();
        }
    }

    if (m_dataMaintainer->isDataNotSaved()) {
        m_dataMaintainer->updateDateTime();

        if (m_settings->instantSaving)
            m_dataMaintainer->saveData();

        sendDbUpdated();
    }
}

void Manager::updateItemFile(const QModelIndex &fileIndex, DbMod job)
{
    const DataContainer *pData = m_dataMaintainer->m_data;
    const FileStatus prevStatus = TreeModel::itemFileStatus(fileIndex);

    if (!pData
        || DataHelper::isImmutable(pData)
        || !(prevStatus & FileStatus::CombUpdatable))
    {
        return;
    }

    if (prevStatus == FileStatus::New) {
        if (job & (DM_ImportDigest | DM_PasteDigest)) {
            const QString dig = (job == DM_ImportDigest) ? extractDigestFromFile(DataHelper::digestFilePath(pData, fileIndex))
                                                         : TreeModel::itemFileChecksum(fileIndex);

            // checking for compliance with the current algo
            if (tools::canBeChecksum(dig, pData->m_metadata.algorithm))
                m_dataMaintainer->importChecksum(fileIndex, dig);
        } else { // calc the new one
            const FileValues fileVal = hashItem(fileIndex);

            if (fileVal.checksum.isEmpty()) { // return previous status
                m_dataMaintainer->setFileStatus(fileIndex, prevStatus);
            } else {
                m_dataMaintainer->updateChecksum(fileIndex, fileVal.checksum);
                m_dataMaintainer->setItemValue(fileIndex, Column::ColumnElapsed, fileVal.hash_time);
                m_dataMaintainer->setItemValue(fileIndex, Column::ColumnSpeed, fileVal.hash_speed());
            }
        }
    } else if (prevStatus == FileStatus::Missing) {
        m_dataMaintainer->itemFileRemoveLost(fileIndex);
    } else if (prevStatus == FileStatus::Mismatched) {
        m_dataMaintainer->itemFileUpdateChecksum(fileIndex);
    }

    if (TreeModel::hasStatus(FileStatus::CombDbChanged, fileIndex)) {
        m_dataMaintainer->setDbFileState(DbFileState::NotSaved);
        m_dataMaintainer->updateNumbers(fileIndex, prevStatus);
        m_dataMaintainer->updateDateTime();

        if (m_settings->instantSaving) {
            saveData();
        } else { // temp solution to update Button info
            emit m_proc->stateChanged();
        }
    }
}

void Manager::importBranch(const QModelIndex &rootFolder)
{
    const int imported = m_dataMaintainer->importBranch(rootFolder);

    if (imported > 0 && m_settings->instantSaving) {
        m_dataMaintainer->saveData();
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
    m_dataMaintainer->forkJsonDb(subfolder);
}

// check only selected file instead of full database verification
void Manager::verifyFileItem(const QModelIndex &fileItemIndex)
{
    if (!m_dataMaintainer->m_data) {
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

    FileValues fileVal = hashItem(fileItemIndex, Verification);

    if (!fileVal.reChecksum.isEmpty()) {
        m_dataMaintainer->updateChecksum(fileItemIndex, fileVal.reChecksum);
        m_dataMaintainer->updateNumbers(fileItemIndex, storedStatus);
        m_dataMaintainer->setItemValue(fileItemIndex, Column::ColumnElapsed, fileVal.hash_time);
        m_dataMaintainer->setItemValue(fileItemIndex, Column::ColumnSpeed, fileVal.hash_speed());

        fileVal.checksum = storedSum.toLower();
        const QString filePath = DataHelper::itemAbsolutePath(m_dataMaintainer->m_data, fileItemIndex);
        emit fileProcessed(filePath, fileVal);
    }
    else if (m_proc->isCanceled()) {
        // return previous status
        m_dataMaintainer->setFileStatus(fileItemIndex, storedStatus);
    }
}

void Manager::verifyFolderItem(const QModelIndex &folderItemIndex, FileStatus checkstatus)
{
    if (!m_dataMaintainer->m_data) {
        return;
    }

    if (!DataHelper::contains(m_dataMaintainer->m_data, FileStatus::CombAvailable, folderItemIndex)) {
        QString warningText = "There are no files available for verification.";
        if (!folderItemIndex.isValid())
            warningText.append("\n\n" + k_movedDbWarning);

        emit showMessage(warningText, "Warning");
        return;
    }

    // main job
    calculateChecksums(checkstatus, folderItemIndex);

    if (m_proc->isCanceled())
        return;

    // changing accompanying statuses to "Matched"
    // item statuses with checksums that can be considered already verified/matched
    FileStatuses accompStatuses = (FileStatus::Added | FileStatus::Updated | FileStatus::Moved);
    if (m_dataMaintainer->m_data->m_numbers.contains(accompStatuses)) {
        m_dataMaintainer->changeStatuses(accompStatuses, FileStatus::Matched, folderItemIndex);
    }

    // result
    if (!folderItemIndex.isValid()) { // if root folder
        emit folderChecked(m_dataMaintainer->m_data->m_numbers);

        // Save the verification datetime, if needed
        if (m_settings->saveVerificationDateTime) {
            m_dataMaintainer->updateVerifDateTime();
        }
    } else { // if subfolder
        const Numbers &nums = DataHelper::getNumbers(m_dataMaintainer->m_data, folderItemIndex);
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
    QString digest;

    try {
        digest = tools::extractDigestFromFile(digest_file);
    }
    catch (const Exception& e) {
        emit showMessage(e.what(), "Error");
    }

    return digest;
}

void Manager::checkFile(const QString &filePath, const QString &checkSum)
{
    checkFile(filePath, checkSum, tools::algoByStrLen(checkSum.length()));
}

void Manager::checkFile(const QString &filePath, const QString &checkSum, QCryptographicHash::Algorithm algo)
{
    FileValues fileVal = hashFile(filePath, algo, Verification);

    if (!m_proc->isCanceled()) {
        fileVal.checksum = checkSum;
        emit fileProcessed(filePath, fileVal);
    }
}

FileValues Manager::hashFile(const QString &filePath, QCryptographicHash::Algorithm algo, const CalcKind calckind)
{
    QFileInfo fi(filePath);
    FileValues fileVal(fi.size());

    if (calckind == Verification)
        fileVal.hash_purpose = FileValues::HashingPurpose::Verify;

    if (!m_proc->hasTotalSize())
        m_proc->setTotalSize(fileVal.size);

    updateProgText(calckind, filePath);

    // hashing
    try {
        m_elapsedTimer.start();

        // automatic choose: '.checksum' or '.reChecksum'
        fileVal.defaultChecksum() = m_shaCalc.calculate(filePath, algo);
        fileVal.hash_time = m_elapsedTimer.elapsed();
    }
    catch (const Exception& e) {
        switch (e.errorCode) {
        case ERR_READ:
            fileVal.status = FileStatus::ReadError;
            qWarning() << "Read ERROR:" << filePath;
            break;
        case ERR_NOPERM:
            fileVal.status = FileStatus::UnPermitted;
            break;
        case ERR_NOTEXIST:
            fileVal.status = (calckind == Verification) ? FileStatus::Missing : FileStatus::Removed;
            break;
        default:
            break;
        }

        if (e.errorCode != ERR_CANCELED)
            emit setStatusbarText(e.what());
    }

    return fileVal;
}

FileValues Manager::hashItem(const QModelIndex &ind, const CalcKind calckind)
{
    m_dataMaintainer->setFileStatus(ind,
                                    calckind ? FileStatus::Verifying : FileStatus::Calculating);

    const QString filePath = DataHelper::itemAbsolutePath(m_dataMaintainer->m_data, ind);
    const FileValues fileVal = hashFile(filePath, m_dataMaintainer->m_data->m_metadata.algorithm, calckind);

    // error handling
    if (fileVal.status & FileStatus::CombCalcError) {
        m_dataMaintainer->setFileStatus(ind, fileVal.status);
    }

    return fileVal;
}

void Manager::updateProgText(const CalcKind calckind, const QString &file)
{
    const QString purp = calckind ? QStringLiteral(u"Verifying") : QStringLiteral(u"Calculating");
    const Chunks<qint64> chunks_size = m_proc->chunksSize();
    const Chunks<int> chunks_queue = m_proc->chunksQueue();

    // single file
    if (!chunks_queue.isSet()) {
        // purp: file (size)
        const QString str = tools::joinStrings(purp,
                                               format::fileNameAndSize(file, chunks_size.total),
                                               Lit::s_sepColonSpace);
        emit setStatusbarText(str);
        return;
    }

    // to avoid calling the dataSizeReadable() func for each file
    static qint64 sLastTotalSize;
    static QString sLastTotalSizeR;

    if (!chunks_size.hasChunks() || (sLastTotalSize != chunks_size.total)) {
        sLastTotalSize = chunks_size.total;
        sLastTotalSizeR = format::dataSizeReadable(sLastTotalSize);
    }

    // UGLY, but better performance (should be). And should be re-implemented.
    const QString res = purp % ' ' % QString::number(chunks_queue.done + 1) % QStringLiteral(u" of ")
                    % QString::number(chunks_queue.total) % QStringLiteral(u" checksums ")
                    % (!chunks_size.hasChunks() ? format::inParentheses(sLastTotalSizeR) // (%1)
                    : ('(' % format::dataSizeReadable(chunks_size.done) % QStringLiteral(u" / ") % sLastTotalSizeR % ')')); // "(%1 / %2)"

    emit setStatusbarText(res);
}

int Manager::calculateChecksums(const FileStatus status, const QModelIndex &root)
{
    return calculateChecksums(DM_AutoSelect, status, root);
}

int Manager::calculateChecksums(const DbMod purpose, const FileStatus status, const QModelIndex &root)
{
    const DataContainer *pData = m_dataMaintainer->m_data;

    if (!pData
        || (root.isValid() && root.model() != pData->m_model))
    {
        qDebug() << "Manager::calculateChecksums | No data or wrong rootIndex";
        return 0;
    }

    if (status != FileStatus::Queued)
        m_dataMaintainer->addToQueue(status, root);

    const NumSize num_queued = DataHelper::getNumbers(pData, root).values(FileStatus::Queued);

    qDebug() << "Manager::calculateChecksums | Queued:" << num_queued.number;

    if (!num_queued)
        return 0;

    m_proc->setTotal(num_queued);

    bool isMismatchFound = false;

    // checking whether this is a Calculation or Verification process
    const CalcKind calc_kind = (status & FileStatus::CombAvailable) ? Verification : Calculation;

    // process
    TreeModelIterator iter(pData->m_model, root);

    while (iter.hasNext() && !m_proc->isCanceled()) {
        if (iter.nextFile().status() != FileStatus::Queued)
            continue;

        // hashing
        const FileValues fileVal = hashItem(iter.index(), calc_kind);
        const QString &sum = fileVal.defaultChecksum();

        if (m_proc->isCanceled())
            break;

        if (sum.isEmpty()) {
            m_proc->decreaseTotalQueued();
            m_proc->decreaseTotalSize(iter.size());
            continue;
        }

        // success
        m_proc->addDoneOne();
        m_dataMaintainer->setItemValue(iter.index(), Column::ColumnElapsed, fileVal.hash_time);
        m_dataMaintainer->setItemValue(iter.index(), Column::ColumnSpeed, fileVal.hash_speed());

        if (purpose == DM_FindMoved) {
            if (!m_dataMaintainer->tryMoved(iter.index(), sum))
                m_dataMaintainer->setFileStatus(iter.index(), status); // rollback status
            continue;
        }

        // != DM_FindMoved
        if (!m_dataMaintainer->updateChecksum(iter.index(), sum)
            && !isMismatchFound) // the signal is only needed once
        {
            emit mismatchFound();
            isMismatchFound = true;
        }
    }

    const int done = m_proc->chunksQueue().done;
    if (m_proc->isCanceled()) {
        if (m_proc->isState(State::Abort)) {
            qDebug() << "Manager::calculateChecksums >> Aborted";
            return 0;
        }

        // rolling back file statuses
        m_dataMaintainer->rollBackStoppedCalc(root, status);
        qDebug() << "Manager::calculateChecksums >> Stopped | Done" << done;
    }

    // end
    m_dataMaintainer->updateNumbers();
    return done;
}

// info about folder (number of files and total size) or file (size)
void Manager::getPathInfo(const QString &path)
{
    if (!m_isViewFileSysytem)
        return;

    QFileInfo fileInfo(path);

    if (fileInfo.isFile()) {
        emit setStatusbarText(format::fileNameAndSize(path));
    } else if (fileInfo.isDir()) {
        emit setStatusbarText(QStringLiteral(u"counting..."));
        emit setStatusbarText(m_files->getFolderSize(path));
    }

}

void Manager::getIndexInfo(const QModelIndex &curIndex)
{
    if (!m_isViewFileSysytem && m_dataMaintainer->m_data)
        emit setStatusbarText(m_dataMaintainer->itemContentsInfo(curIndex));
}

void Manager::folderContentsList(const QString &folderPath, bool filterCreation)
{
    if (!m_isViewFileSysytem)
        return;

    if (Files::isEmptyFolder(folderPath)) {
        emit showMessage("There are no file types to display.", "Empty folder");
        return;
    }

    FilterRule comb_attr(FilterAttribute::NoAttributes);
    if (m_settings->filter_ignore_unpermitted)
        comb_attr.addAttribute(FilterAttribute::IgnoreUnpermitted);
    if (m_settings->filter_ignore_symlinks)
        comb_attr.addAttribute(FilterAttribute::IgnoreSymlinks);

    const FileTypeList typesList = m_files->getFileTypes(folderPath, comb_attr);

    if (!typesList.isEmpty()) {
        if (filterCreation)
            emit dbCreationDataCollected(folderPath, Files::dbFiles(folderPath), typesList);
        else
            emit folderContentsListCreated(folderPath, typesList);
    }

}

void Manager::makeDbContentsList()
{
    const DataContainer *pData = m_dataMaintainer->m_data;

    if (!pData)
        return;

    const FileTypeList typesList = m_files->getFileTypes(pData->m_model);

    if (!typesList.isEmpty())
        emit dbContentsListCreated(pData->m_metadata.workDir, typesList);
}

void Manager::cacheMissingItems()
{
    DataContainer *pData = m_dataMaintainer->m_data;

    if (!pData || !DataHelper::hasPossiblyMovedItems(pData)) {
        return;
    }

    TreeModelIterator it(pData->m_model);

    while (it.hasNext()) {
        if (it.nextFile().status() == FileStatus::Missing) {
            const QString dig = it.checksum();
            if (!pData->m_cacheMissing.contains(dig))
                pData->m_cacheMissing[dig] = it.index();
        }
    }

    qDebug() << Q_FUNC_INFO << "Cached:" << pData->m_cacheMissing.size();
}

void Manager::modelChanged(ModelView modelView)
{
    m_isViewFileSysytem = (modelView == ModelView::FileSystem);
}
