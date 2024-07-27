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
#include "files.h"
#include "shacalculator.h"
#include "treemodeliterator.h"
#include "tools.h"

Manager::Manager(Settings *settings, QObject *parent)
    : QObject(parent), settings_(settings)
{
    files_->setProcState(procState);
    dataMaintainer->setProcState(procState);

    connect(dataMaintainer, &DataMaintainer::showMessage, this, &Manager::showMessage);
    connect(dataMaintainer, &DataMaintainer::setStatusbarText, this, &Manager::setStatusbarText);
    connect(files_, &Files::setStatusbarText, this, &Manager::setStatusbarText);
}

void Manager::runTask(std::function<void()> task)
{
    procState->setState(State::StartSilently);

    task();

    procState->setState(State::Idle);
}

void Manager::processFolderSha(const MetaData &metaData)
{
    runTask([&] { _processFolderSha(metaData); });
}

void Manager::_processFolderSha(const MetaData &metaData)
{
    if (Files::isEmptyFolder(metaData.workDir, metaData.filter)) {
        emit showMessage("All files have been excluded.\n"
                         "Filtering rules can be changed in the settings.", "No proper files");
        return;
    }

    dataMaintainer->setSourceData();
    dataMaintainer->data_->metaData = metaData;
    dataMaintainer->data_->model_->setColoredItems(settings_->coloredDbItems);
    dataMaintainer->setDbFileState(MetaData::NoFile);

    // create the filelist
    dataMaintainer->addActualFiles(FileStatus::Queued, false);

    // exception and cancelation handling
    if (procState->isCanceled() || !dataMaintainer->data_) {
        emit setViewData();
        return;
    }

    emit setViewData(dataMaintainer->data_);

    // calculating checksums
    calculateChecksums(FileStatus::Queued);

    if (!procState->isCanceled()) { // saving to json
        dataMaintainer->updateDateTime();
        dataMaintainer->exportToJson();
        QTimer::singleShot(0, dataMaintainer, &DataMaintainer::databaseUpdated);
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

void Manager::resetDatabase()
{
    if (dataMaintainer->data_)
        createDataModel(dataMaintainer->data_->metaData.databaseFilePath);
}

void Manager::restoreDatabase()
{
    if (dataMaintainer->data_
        && (dataMaintainer->data_->restoreBackupFile() || dataMaintainer->isDataNotSaved()))
    {
        runTask([&] { _createDataModel(dataMaintainer->data_->metaData.databaseFilePath); });
    }
    else
        emit setStatusbarText("No saved changes");
}

void Manager::createDataModel(const QString &databaseFilePath)
{
    if (!tools::isDatabaseFile(databaseFilePath)) {
        emit showMessage(QString("Wrong file: %1\nExpected file extension '*.ver' or '*.ver.json'").arg(databaseFilePath), "Wrong DB file!");
        emit setViewData();
        return;
    }

    runTask([&] { dataMaintainer->saveData(); _createDataModel(databaseFilePath); });
}

void Manager::_createDataModel(const QString &databaseFilePath)
{
    if (dataMaintainer->importJson(databaseFilePath)) {
        dataMaintainer->data_->model_->setColoredItems(settings_->coloredDbItems);
        emit setViewData(dataMaintainer->data_);
    }
    else
        emit setViewData();
}

void Manager::saveData()
{
    if (dataMaintainer->isDataNotSaved())
        runTask([&] { dataMaintainer->saveData(); });
}

void Manager::prepareSwitchToFs()
{
    saveData();

    if (dataMaintainer->isDataNotSaved())
        emit showMessage("The Database is NOT saved", "Error");

    emit switchToFsPrepared();

    qDebug() << "Manager::prepareSwitchToFs >> DONE";
}

void Manager::updateDatabase(const DestDbUpdate dest)
{
    runTask([&] { _updateDatabase(dest); });
}

void Manager::_updateDatabase(const DestDbUpdate dest)
{
    if (!dataMaintainer->data_)
        return;

    if (!dataMaintainer->data_->contains(FileStatus::FlagAvailable)) {
        emit showMessage("Failure to delete all database items.\n\n" + movedDbWarning, "Warning");
        return;
    }

    if (dest == DestUpdateMismatches)
        dataMaintainer->updateMismatchedChecksums();

    else {
        if ((dest & DestAddNew)
            && dataMaintainer->data_->contains(FileStatus::New)) {

            int numAdded = calculateChecksums(FileStatus::New); // !!! here was the only place where <finalProcess = false> was used

            if (procState->isCanceled())
                return;
            else if (numAdded > 0)
                dataMaintainer->setDbFileState(DbFileState::NotSaved);
        }

        if ((dest & DestClearLost)
            && dataMaintainer->data_->contains(FileStatus::Missing)) {

            dataMaintainer->clearLostFiles();
        }
    }

    if (dataMaintainer->isDataNotSaved()) {
        dataMaintainer->updateDateTime();

        if (settings_->instantSaving)
            dataMaintainer->saveData();            

        QTimer::singleShot(0, dataMaintainer, &DataMaintainer::databaseUpdated);
    }
}

void Manager::updateItemFile(const QModelIndex &fileIndex)
{
    if (!dataMaintainer->data_ || !fileIndex.isValid()) {
        return;
    }

    FileStatus fileStatusBefore = TreeModel::itemFileStatus(fileIndex);

    if (fileStatusBefore == FileStatus::New) {
        dataMaintainer->data_->model_->setRowData(fileIndex, Column::ColumnStatus, FileStatus::Calculating);
        QString filePath = dataMaintainer->data_->itemAbsolutePath(fileIndex);
        QString sum = calculateChecksum(filePath, dataMaintainer->data_->metaData.algorithm);

        if (sum.isEmpty()) { // return previous status
            dataMaintainer->data_->model_->setRowData(fileIndex, Column::ColumnStatus, FileStatus::New);
        }
        else
            dataMaintainer->updateChecksum(fileIndex, sum);
    }
    else if (fileStatusBefore == FileStatus::Missing) {
        dataMaintainer->itemFileRemoveLost(fileIndex);
    }
    else if (fileStatusBefore == FileStatus::Mismatched) {
        dataMaintainer->itemFileUpdateChecksum(fileIndex);
    }

    if (TreeModel::hasStatus(FileStatus::FlagDbChanged, fileIndex)) {
        dataMaintainer->setDbFileState(DbFileState::NotSaved);
        dataMaintainer->updateNumbers(fileIndex, fileStatusBefore);
        dataMaintainer->updateDateTime();

        if (settings_->instantSaving)
            saveData();
        else
            emit procState->stateChanged(); // temp solution to update Button info
    }
}

void Manager::branchSubfolder(const QModelIndex &subfolder)
{
    runTask([&] { dataMaintainer->forkJsonDb(subfolder); });
}

void Manager::verify(const QModelIndex &curIndex)
{
    if (TreeModel::isFileRow(curIndex))
        verifyFileItem(curIndex);
    else
        verifyFolderItem(curIndex);
}

// check only selected file instead of full database verification
void Manager::verifyFileItem(const QModelIndex &fileItemIndex)
{
    if (!dataMaintainer->data_) {
        return;
    }

    FileStatus storedStatus = TreeModel::itemFileStatus(fileItemIndex);
    QString storedSum = TreeModel::itemFileChecksum(fileItemIndex);

    if (!tools::canBeChecksum(storedSum) || !(storedStatus & FileStatus::FlagAvailable)) {
        switch (storedStatus) {
        case FileStatus::New:
            emit showMessage("The checksum is not yet in the database.\nPlease Update New/Lost", "New File");
            break;
        case FileStatus::Missing:
            emit showMessage("File does not exist", "Missing File");
            break;
        case FileStatus::Removed:
            emit showMessage("The checksum has been removed from the database.", "No Checksum");
            break;
        case FileStatus::Unreadable:
            emit showMessage("This file has been excluded (Unreadable).\nNo checksum in the database.", "Unreadable File");
            break;
        default:
            emit showMessage("No checksum in the database.", "No Checksum");
            break;
        }

        return;
    }

    dataMaintainer->data_->model_->setRowData(fileItemIndex, Column::ColumnStatus, FileStatus::Verifying);
    QString filePath = dataMaintainer->data_->itemAbsolutePath(fileItemIndex);
    QString sum = calculateChecksum(filePath, dataMaintainer->data_->metaData.algorithm, true);

    if (sum.isEmpty()) { // return previous status
        dataMaintainer->data_->model_->setRowData(fileItemIndex, Column::ColumnStatus, storedStatus);
    }
    else {
        showFileCheckResultMessage(filePath, storedSum, sum);
        dataMaintainer->updateChecksum(fileItemIndex, sum);
        dataMaintainer->updateNumbers(fileItemIndex, storedStatus);
    }
}

void Manager::verifyFolderItem(const QModelIndex &folderItemIndex)
{
    runTask([&] { _verifyFolderItem(folderItemIndex); });
}

void Manager::_verifyFolderItem(const QModelIndex &folderItemIndex)
{
    if (!dataMaintainer->data_)
        return;

    if (!dataMaintainer->data_->contains(FileStatus::FlagAvailable, folderItemIndex)) {
        QString warningText = "There are no files available for verification.";
        if (!folderItemIndex.isValid())
            warningText.append("\n\n" + movedDbWarning);

        emit showMessage(warningText, "Warning");
        return;
    }

    // main job
    calculateChecksums(folderItemIndex, FileStatus::NotChecked);

    if (procState->isCanceled())
        return;

    // changing accompanying statuses to "Matched"
    FileStatuses flagAddedUpdated = (FileStatus::Added | FileStatus::Updated);
    if (dataMaintainer->data_->numbers.contains(flagAddedUpdated)) {
        dataMaintainer->changeFilesStatus(flagAddedUpdated, FileStatus::Matched, folderItemIndex);
    }

    // result
    if (!folderItemIndex.isValid()) { // if root folder
        emit folderChecked(dataMaintainer->data_->numbers);

        // Save the verification datetime, if needed
        if (settings_->saveVerificationDateTime) {
            dataMaintainer->updateVerifDateTime();
        }
    }
    else { // if subfolder
        Numbers num = dataMaintainer->data_->getNumbers(folderItemIndex);
        QString subfolderName = TreeModel::itemName(folderItemIndex);

        emit folderChecked(num, subfolderName);
    }
}

void Manager::checkSummaryFile(const QString &path)
{
    QFile sumFile(path);
    QString line;
    if (sumFile.open(QFile::ReadOnly))
        line = sumFile.readLine();
    else {
        emit showMessage("Error while reading Summary File", "Error");
        return;
    }

    QFileInfo fileInfo(path);
    QString storedChecksum;

    if (line.contains("  ") && tools::canBeChecksum(line.left(line.indexOf("  "))))
        storedChecksum = line.left(line.indexOf("  "));
    else if (line.contains(" *") && tools::canBeChecksum(line.left(line.indexOf(" *"))))
        storedChecksum = line.left(line.indexOf(" *"));
    else if (tools::canBeChecksum(line.left(tools::algoStrLen(tools::strToAlgo(fileInfo.suffix())))))
        storedChecksum = line.left(tools::algoStrLen(tools::strToAlgo(fileInfo.suffix())));

    if (storedChecksum.isEmpty()) {
        emit showMessage("Checksum not found", "Warning");
        return;
    }

    QString checkFileName = fileInfo.completeBaseName();
    QString checkFilePath = paths::joinPath(paths::parentFolder(path), checkFileName);

    if (!QFileInfo(checkFilePath).isFile()) {
        checkFilePath = paths::joinPath(paths::parentFolder(path), line.mid(storedChecksum.length() + 2).remove("\n"));
        if (!QFileInfo(checkFilePath).isFile()) {
            emit showMessage("No File to check", "Warning");
            return;
        }
    }

    checkFile(checkFilePath, storedChecksum);
}

void Manager::checkFile(const QString &filePath, const QString &checkSum)
{
    checkFile(filePath, checkSum, tools::algorithmByStrLen(checkSum.length()));
}

void Manager::checkFile(const QString &filePath, const QString &checkSum, QCryptographicHash::Algorithm algo)
{
    QString sum = calculateChecksum(filePath, algo, true);

    if (!sum.isEmpty()) {
        showFileCheckResultMessage(filePath, checkSum, sum);
    }
}

QString Manager::calculateChecksum(const QString &filePath, QCryptographicHash::Algorithm algo, bool isVerification)
{
    procState->setTotalSize(QFileInfo(filePath).size());

    ShaCalculator shaCalc(algo);
    shaCalc.setProcState(procState);

    connect(&shaCalc, &ShaCalculator::doneChunk, procState, &ProcState::addChunk);

    emit setStatusbarText(QString("%1 %2: %3").arg(isVerification ? "Verifying" : "Calculating",
                                                    format::algoToStr(algo),
                                                    format::fileNameAndSize(filePath)));

    QString checkSum = shaCalc.calculate(filePath, algo);

    if (checkSum.isEmpty() && !procState->isCanceled())
        emit showMessage("Read error:\n" + filePath, "Warning");

    // to update the statusbar text
    if (isViewFileSysytem && settings_->lastFsPath) // to prevent unnecessary cancellation
        getPathInfo(*settings_->lastFsPath);
    else
        emit finishedCalcFileChecksum();

    procState->setState(State::Idle);

    return checkSum;
}

int Manager::calculateChecksums(FileStatus status)
{
    return calculateChecksums(QModelIndex(), status);
}

int Manager::calculateChecksums(const QModelIndex &rootIndex, FileStatus status)
{
    if (!dataMaintainer->data_
        || (rootIndex.isValid() && rootIndex.model() != dataMaintainer->data_->model_))
    {
        qDebug() << "Manager::calculateChecksums | No data or wrong rootIndex";
        return 0;
    }

    int numQueued = (status == FileStatus::Queued) ? dataMaintainer->data_->numbers.numberOf(FileStatus::Queued)
                                                   : dataMaintainer->addToQueue(status, rootIndex);

    if (numQueued == 0) {
        qDebug() << "Manager::calculateChecksums | No files in queue";
        return 0;
    }

    qint64 totalSize = dataMaintainer->totalSizeOfListedFiles(FileStatus::Queued, rootIndex);
    QString totalSizeReadable = format::dataSizeReadable(totalSize);
    procState->setTotalSize(totalSize);

    ShaCalculator shaCalc(dataMaintainer->data_->metaData.algorithm);
    shaCalc.setProcState(procState);
    int doneNum = 0;
    bool isMismatchFound = false;

    connect(&shaCalc, &ShaCalculator::doneChunk, procState, &ProcState::addChunk);

    // checking whether this is a Calculation or Verification process
    const FileStatus procStatus = (status & FileStatus::FlagAvailable) ? FileStatus::Verifying : FileStatus::Calculating;
    const QString procStatusText = (procStatus == FileStatus::Verifying) ? "Verifying" : "Calculating";

    // process
    TreeModelIterator iter(dataMaintainer->data_->model_, rootIndex);

    while (iter.hasNext() && !procState->isCanceled()) {
        if (iter.nextFile().status() == FileStatus::Queued) {
            dataMaintainer->data_->model_->setRowData(iter.index(), Column::ColumnStatus, procStatus);

            QString doneData = (procState->doneSize() == 0) ? QString("(%1)").arg(totalSizeReadable)
                                                            : QString("(%1 / %2)").arg(format::dataSizeReadable(procState->doneSize()), totalSizeReadable);

            emit setStatusbarText(QString("%1 %2 of %3 checksums %4")
                                      .arg(procStatusText)
                                      .arg(doneNum + 1)
                                      .arg(numQueued)
                                      .arg(doneData));

            QString curFilePath = dataMaintainer->data_->itemAbsolutePath(iter.index());
            QString checksum = shaCalc.calculate(curFilePath);

            if (!procState->isCanceled()) {
                if (checksum.isEmpty()) {
                    FileStatus _unread = FileStatus::Unreadable;
                    if (!QFileInfo::exists(curFilePath)) {
                        _unread = TreeModel::hasChecksum(iter.index()) ? FileStatus::Missing : FileStatus::Removed;
                    }

                    dataMaintainer->data_->model_->setRowData(iter.index(), Column::ColumnStatus,  _unread);

                    totalSize -= iter.size();
                    totalSizeReadable = format::dataSizeReadable(totalSize);
                    procState->changeTotalSize(totalSize);
                    --numQueued;
                }
                else {
                    if (!dataMaintainer->updateChecksum(iter.index(), checksum)) {
                        if (!isMismatchFound) { // the signal is only needed once
                            emit mismatchFound();
                            isMismatchFound = true;
                        }
                    }
                    ++doneNum;
                }
            }
        }
    }

    if (procState->isState(State::Abort)) {
        qDebug() << "Manager::calculateChecksums >> Aborted";
        return 0;
    }

    // rolling back file statuses
    if (procState->isCanceled()) {
        if (status != FileStatus::Queued) {
            if (status == FileStatus::New)
                dataMaintainer->clearChecksums(FileStatus::Added, rootIndex);

            dataMaintainer->changeFilesStatus((FileStatus::FlagProcessing | FileStatus::Added), status, rootIndex);
        }

        qDebug() << "Manager::calculateChecksums | Stoped | Done" << doneNum;
    }

    // end
    dataMaintainer->updateNumbers();
    return doneNum;
}

void Manager::showFileCheckResultMessage(const QString &filePath, const QString &checksumEstimated, const QString &checksumCalculated)
{
    FileStatus status = (checksumEstimated.toLower() == checksumCalculated.toLower()) ? FileStatus::Matched : FileStatus::Mismatched;
    FileValues fileVal(status, QFileInfo(filePath).size());
    fileVal.checksum = checksumEstimated.toLower();

    if (fileVal.status == FileStatus::Mismatched) {
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
            emit setStatusbarText("counting...");
            runTask([&] { emit setStatusbarText(files_->getFolderSize(path)); });
        }
    }
}

void Manager::getIndexInfo(const QModelIndex &curIndex)
{
    if (!isViewFileSysytem && dataMaintainer->data_)
        emit setStatusbarText(dataMaintainer->itemContentsInfo(curIndex));
}

void Manager::makeFolderContentsList(const QString &folderPath)
{
    runTask([&]{ _folderContentsList(folderPath, false); });
}

void Manager::makeFolderContentsFilter(const QString &folderPath)
{
    runTask([&]{ _folderContentsList(folderPath, true); });
}

void Manager::_folderContentsList(const QString &folderPath, bool filterCreation)
{
    if (isViewFileSysytem) {
        if (Files::isEmptyFolder(folderPath)) {
            emit showMessage("There are no file types to display.", "Empty folder");
            return;
        }

        QList<ExtNumSize> typesList = files_->getFileTypes(folderPath);

        if (!typesList.isEmpty()) {
            if (filterCreation)
                emit folderContentsFilterCreated(folderPath, typesList);
            else
                emit folderContentsListCreated(folderPath, typesList);
        }
    }
}

void Manager::makeDbContentsList()
{
    runTask([&]{ _dbContentsList(); });
}

void Manager::_dbContentsList()
{
    if (!dataMaintainer->data_)
        return;

    QList<ExtNumSize> typesList = files_->getFileTypes(dataMaintainer->data_->model_);

    if (!typesList.isEmpty())
        emit dbContentsListCreated(dataMaintainer->data_->metaData.workDir, typesList);
}

void Manager::modelChanged(ModelView modelView)
{
    isViewFileSysytem = (modelView == ModelView::FileSystem);
}
