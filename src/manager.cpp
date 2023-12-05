// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "manager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QThread>
#include <QGuiApplication>
#include <QClipboard>
#include <QDebug>
#include "files.h"
#include "shacalculator.h"
#include "procstate.h"
#include "treemodeliterator.h"

Manager::Manager(Settings *settings, QObject *parent)
    : QObject(parent), settings_(settings)
{
    connect(this, &Manager::cancelProcess, this, [=]{canceled = true; emit setStatusbarText("Canceled");}, Qt::DirectConnection);

    connect(this, &Manager::cancelProcess, dataMaintainer, &DataMaintainer::cancelProcess, Qt::DirectConnection);
    connect(dataMaintainer, &DataMaintainer::processing, this, &Manager::processing);
    connect(dataMaintainer, &DataMaintainer::showMessage, this, &Manager::showMessage);
    connect(dataMaintainer, &DataMaintainer::setStatusbarText, this, &Manager::setStatusbarText);
    connect(dataMaintainer, &DataMaintainer::setPermanentStatus, this, &Manager::setPermanentStatus);
}

void Manager::processFolderSha(const QString &folderPath, QCryptographicHash::Algorithm algo)
{
    if (!Files::containsFiles(folderPath)) {
        emit showMessage("Empty folder. Nothing to do");
        return;
    }

    if (!Files::containsFiles(folderPath, settings_->filter)) {
        emit showMessage("All files have been excluded.\nFiltering rules can be changed in the settings.");
        return;
    }

    canceled = false;

    dataMaintainer->setSourceData();
    dataMaintainer->data_->metaData.workDir = folderPath;
    dataMaintainer->data_->metaData.algorithm = algo;
    dataMaintainer->data_->metaData.filter = settings_->filter;
    dataMaintainer->data_->metaData.databaseFilePath = paths::joinPath(folderPath,
                                                       QString("%1_%2.ver.json").arg(settings_->dbPrefix, paths::basicName(folderPath)) // database filename
                                                                                .replace(' ', '_'));

    // create the filelist
    dataMaintainer->addActualFiles(FileStatus::Queued, false);

    // exception and cancelation handling
    if (canceled || !dataMaintainer->data_) {
        emit setViewData();
        return;
    }

    emit setViewData(dataMaintainer->data_, ModelView::ModelSource);

    QString permStatus = format::algoToStr(algo);
    if (dataMaintainer->data_->isFilterApplied())
        permStatus.prepend("filters applied | ");

    emit setPermanentStatus(permStatus);

    // calculating checksums
    int doneNumber = calculateChecksums();

    if (canceled) {
        emit setViewData();
        emit setPermanentStatus();
        return;
    }

    // saving to json
    dataMaintainer->data_->metaData.about = QString("%1 Checksums for %2 files calculated")
                                                    .arg(format::algoToStr(algo))
                                                    .arg(doneNumber);

    dataMaintainer->exportToJson();
}

void Manager::processFileSha(const QString &filePath, QCryptographicHash::Algorithm algo, bool summaryFile, bool clipboard)
{
    QString sum = calculateChecksum(filePath, algo);
    if (sum.isEmpty())
        return;

    if (clipboard) {
        QGuiApplication::clipboard()->setText(sum);
        emit showMessage(QString("Computed checksum is copied to clipboard\n\n%1: %2")
                        .arg(format::algoToStr(algo), format::shortenString(sum, 40)), paths::basicName(filePath));
    }

    if (summaryFile) {
        QString ext;

        switch (algo) {
            case QCryptographicHash::Sha1:
                ext = "sha1";
                break;
            case QCryptographicHash::Sha256:
                ext = "sha256";
                break;
            case QCryptographicHash::Sha512:
                ext = "sha512";
                break;
            default:
                ext = "checkSum";
                break;
        }

        QString sumFile = QString("%1.%2").arg(filePath, ext);
        QFile file(sumFile);
        if (file.open(QFile::WriteOnly)) {
            file.write(QString("%1 *%2").arg(sum, paths::basicName(filePath)).toUtf8());
            emit showMessage(QString("The checksum is saved in the summary file:\n\n%1").arg(paths::basicName(sumFile)), // Message body
                             QString("%1 computed").arg(format::algoToStr(algo))); // Message header
        }
        else {
            QGuiApplication::clipboard()->setText(sum); // if unable to write summary, send the checksum to clipboard
            emit showMessage(QString("Unable to create summary file: %1\nChecksum is copied to clipboard\n\n%2: %3")
                                  .arg(sumFile, format::algoToStr(algo), format::shortenString(sum, 40)), "Warning");
        }
    }
}

void Manager::resetDatabase()
{
    if (dataMaintainer->data_)
        createDataModel(dataMaintainer->data_->metaData.databaseFilePath);
}

void Manager::restoreDatabase()
{
    if (dataMaintainer->data_ && dataMaintainer->data_->restoreBackupFile())
        createDataModel(dataMaintainer->data_->metaData.databaseFilePath);
    else
        emit setStatusbarText("No saved changes");
}

//making tree model | file paths : info about current availability on disk
void Manager::createDataModel(const QString &databaseFilePath)
{
    if (!tools::isDatabaseFile(databaseFilePath)) {
        emit showMessage(QString("Wrong file: %1\nExpected file extension '*.ver.json'").arg(databaseFilePath), "Wrong DB file!");
        emit setViewData();
        return;
    }

    if (dataMaintainer->importJson(databaseFilePath))
        emit setViewData(dataMaintainer->data_);
}

void Manager::updateNewLost()
{
    if (!dataMaintainer->data_) {
        emit processing(false);
        return;
    }

    QString itemsInfo;

    int numNew = dataMaintainer->data_->numbers.numberOf(FileStatus::New);
    int numMissing = dataMaintainer->data_->numbers.numberOf(FileStatus::Missing);

    if (numMissing > 0 && numMissing >= (dataMaintainer->data_->numbers.numChecksums + numNew)) {
        emit processing(false);
        emit showMessage("Failure to delete all database items", "Warning");
        return;
    }

    if (numNew > 0) {
        calculateChecksums(FileStatus::New);

        if (canceled)
            return;

        itemsInfo.append(QString("added %1").arg(dataMaintainer->data_->numbers.numberOf(FileStatus::Added)));

        if (numMissing > 0)
            itemsInfo.append(", ");
    }

    if (numMissing > 0) {
        itemsInfo.append(QString("removed %1").arg(dataMaintainer->clearDataFromLostFiles()));
    }

    dataMaintainer->data_->metaData.about = QString("Database updated: %1 items").arg(itemsInfo);

    dataMaintainer->exportToJson();
}

// update the Database with new checksums for files with failed verification
void Manager::updateMismatch()
{
    if (!dataMaintainer->data_) {
        emit processing(false);
        return;
    }

    dataMaintainer->data_->metaData.about = QString("%1 checksums updated")
                                            .arg(dataMaintainer->updateMismatchedChecksums());

    dataMaintainer->exportToJson();
}

void Manager::verify(const QModelIndex &curIndex)
{
    TreeModel::isFileRow(curIndex) ? verifyFileItem(curIndex)
                                   : verifyFolderItem(dataMaintainer->sourceIndex(curIndex));
}

//check only selected file instead all database cheking
void Manager::verifyFileItem(const QModelIndex &fileItemIndex)
{
    if (!dataMaintainer->data_) {
        emit processing(false);
        return;
    }

    FileStatus storedStatus = TreeModel::siblingAtRow(fileItemIndex, Column::ColumnStatus).data(TreeModel::RawDataRole).value<FileStatus>();

    if (storedStatus == FileStatus::Missing) {
        emit showMessage("File does not exist", "Missing file");
        return;
    }

    QString savedSum = dataMaintainer->getStoredChecksum(fileItemIndex);

    if (!savedSum.isEmpty()) {
        dataMaintainer->data_->model_->setItemData(fileItemIndex, Column::ColumnStatus, FileStatus::Processing);

        QString sum = calculateChecksum(paths::joinPath(dataMaintainer->data_->metaData.workDir, TreeModel::getPath(fileItemIndex)),
                                        dataMaintainer->data_->metaData.algorithm);

        if (sum.isEmpty()) {
            dataMaintainer->data_->model_->setItemData(fileItemIndex, Column::ColumnStatus, storedStatus);
        }
        else {
            showFileCheckResultMessage(dataMaintainer->updateChecksum(fileItemIndex, sum));
            dataMaintainer->updateNumbers();
        }
    }
}

void Manager::verifyFolderItem(const QModelIndex &folderItemIndex)
{
    if (!dataMaintainer->data_) {
        emit processing(false);
        return;
    }

    dataMaintainer->changeFilesStatus({FileStatus::Added, FileStatus::ChecksumUpdated}, FileStatus::Matched, folderItemIndex);
    calculateChecksums(folderItemIndex, FileStatus::NotChecked);

    if (canceled) {
        return;
    }

    // result
    if (!folderItemIndex.isValid()) { // if root folder
        int numMatched = dataMaintainer->data_->numbers.numberOf(FileStatus::Matched);

        if (dataMaintainer->data_->numbers.numberOf(FileStatus::Mismatched) > 0)
            emit showMessage(QString("%1 out of %2 files is changed or corrupted")
                                 .arg(dataMaintainer->data_->numbers.numberOf(FileStatus::Mismatched))
                                 .arg(dataMaintainer->data_->numbers.available()), "FAILED");
        else if (numMatched > 0)
            emit showMessage(QString("ALL %1 files passed the verification.\nStored %2 checksums matched.")
                                 .arg(numMatched)
                                 .arg(format::algoToStr(dataMaintainer->data_->metaData.algorithm)), "Success");
    }
    else {// if subfolder
        QString subfolderName = TreeModel::siblingAtRow(folderItemIndex, Column::ColumnPath).data().toString();
        Numbers num = dataMaintainer->updateNumbers(dataMaintainer->data_->model_, folderItemIndex);
        int subMatched = num.numberOf(FileStatus::Matched);

        if (num.numberOf(FileStatus::Mismatched) > 0)
            emit showMessage(QString("Subfolder: %1\n\n%2 out of %3 files in the Subfolder is changed or corrupted")
                                 .arg(subfolderName)
                                 .arg(num.numberOf(FileStatus::Mismatched))
                                 .arg(subMatched + num.numberOf(FileStatus::Mismatched)), "FAILED");
        else
            emit showMessage(QString("Subfolder: %1\n\nAll %2 checked files passed the verification")
                                 .arg(subfolderName)
                                 .arg(subMatched), "Success");
    }
}

void Manager::checkSummaryFile(const QString &path)
{
    QFileInfo fileInfo(path);
    QCryptographicHash::Algorithm algo = tools::strToAlgo(fileInfo.suffix());
    QString checkSummaryFileName = fileInfo.completeBaseName();
    QString checkSummaryFilePath = paths::joinPath(paths::parentFolder(path), checkSummaryFileName);

    QFile sumFile(path);
    QString line;
    if (sumFile.open(QFile::ReadOnly))
        line = sumFile.readLine();
    else {
        emit showMessage("Error while reading Summary File", "Error");
        return;
    }

    if (!QFileInfo(checkSummaryFilePath).isFile()) {
        checkSummaryFilePath = paths::joinPath(paths::parentFolder(path), line.mid(tools::algoStrLen(algo) + 2).remove("\n"));
        if (!QFileInfo(checkSummaryFilePath).isFile()) {
            emit showMessage("No File to check", "Warning");
            return;
        }
    }

    checkFile(checkSummaryFilePath, line.mid(0, tools::algoStrLen(algo)), algo);
}

void Manager::checkFile(const QString &filePath, const QString &checkSum)
{
    checkFile(filePath, checkSum, tools::algorithmByStrLen(checkSum.length()));
}

void Manager::checkFile(const QString &filePath, const QString &checkSum, QCryptographicHash::Algorithm algo)
{
    QString sum = calculateChecksum(filePath, algo);

    if (!sum.isEmpty())
        showFileCheckResultMessage(sum == checkSum.toLower(), paths::basicName(filePath));
}

QString Manager::calculateChecksum(const QString &filePath, QCryptographicHash::Algorithm algo, bool finalProcess)
{
    ProcState state(QFileInfo(filePath).size());
    ShaCalculator shaCalc(algo);

    connect(this, &Manager::cancelProcess, &shaCalc, &ShaCalculator::cancelProcess, Qt::DirectConnection);
    connect(&shaCalc, &ShaCalculator::doneChunk, &state, &ProcState::doneChunk);
    connect(&state, &ProcState::donePercents, this, &Manager::donePercents);
    connect(&state, &ProcState::procStatus, this, &Manager::procStatus);

    emit setStatusbarText(QString("Calculating %1 checksum: %2").arg(format::algoToStr(algo), format::fileNameAndSize(filePath)));

    emit processing(true, true);
    QString checkSum = shaCalc.calculate(filePath, algo);

    if (!checkSum.isEmpty())
        emit setStatusbarText(QString("%1 calculated").arg(format::algoToStr(algo)));
    else if (!canceled)
        emit setStatusbarText("read error");

    if (finalProcess)
        emit processing(false);

    return checkSum;
}

int Manager::calculateChecksums(FileStatus status, bool finalProcess)
{
    return calculateChecksums(QModelIndex(), status, finalProcess);
}

int Manager::calculateChecksums(QModelIndex rootIndex, FileStatus status, bool finalProcess)
{
    if (!dataMaintainer->data_) {
        qDebug() << "Manager::calculateChecksums | No data";
        return 0;
    }

    if (rootIndex.isValid() && rootIndex.model() == dataMaintainer->data_->proxyModel_)
        rootIndex = dataMaintainer->data_->proxyModel_->mapToSource(rootIndex);

    /* testing: ModeSelector::prepareView() override this signal
    emit processing(true); // The main signal is located below, and this one is needed for the correct return to the ProxyModel in case numQueued == 0
                           // This signal can be removed by adding the same to the functions below (numberOf() and addToQueue()/changeFilesStatus())
    */

    int numQueued = (status == FileStatus::Queued) ? dataMaintainer->data_->numbers.numberOf(FileStatus::Queued)
                                                   : dataMaintainer->addToQueue(status, rootIndex);

    if (numQueued == 0) {
        qDebug() << "Manager::calculateChecksums | No files in queue";
        if (finalProcess)
            emit processing(false);
        return 0;
    }

    qint64 totalSize = dataMaintainer->totalSizeOfListedFiles(FileStatus::Queued, rootIndex);
    QString totalSizeReadable = format::dataSizeReadable(totalSize);
    ProcState state(totalSize);
    ShaCalculator shaCalc(dataMaintainer->data_->metaData.algorithm);
    canceled = false;
    int doneNum = 0;

    connect(this, &Manager::cancelProcess, &shaCalc, &ShaCalculator::cancelProcess, Qt::DirectConnection);
    connect(&shaCalc, &ShaCalculator::doneChunk, &state, &ProcState::doneChunk);
    connect(&state, &ProcState::donePercents, this, &Manager::donePercents);
    connect(&state, &ProcState::procStatus, this, &Manager::procStatus);

    emit processing(true, true); // set processing view, show progress bar

    TreeModelIterator iter(dataMaintainer->data_->model_, rootIndex);

    while (iter.hasNext()) {
        if (iter.nextFile().status() == FileStatus::Queued) {

            QString doneData;
            if (state.doneSize_ == 0)
                doneData = QString("(%1)").arg(totalSizeReadable);
            else
                doneData = QString("(%1 / %2)").arg(format::dataSizeReadable(state.doneSize_), totalSizeReadable);

            emit setStatusbarText(QString("Calculating %1 of %2 checksums %3")
                                      .arg(doneNum + 1)
                                      .arg(numQueued)
                                      .arg(doneData));

            dataMaintainer->data_->model_->setItemData(iter.index(), Column::ColumnStatus, FileStatus::Processing);

            QString checksum = shaCalc.calculate(paths::joinPath(dataMaintainer->data_->metaData.workDir, iter.path()),
                                                 dataMaintainer->data_->metaData.algorithm);

            if (canceled) {
                dataMaintainer->data_->model_->setItemData(iter.index(), Column::ColumnStatus, status);

                if (status != FileStatus::Queued) {
                    dataMaintainer->clearChecksums(FileStatus::Added, rootIndex);
                    dataMaintainer->changeFilesStatus({FileStatus::Queued, FileStatus::Added}, status, rootIndex);
                }

                qDebug() << "Manager::calculateChecksums | CANCELED | Done" << doneNum;
                break;
            }

            if (checksum.isEmpty())
                dataMaintainer->data_->model_->setItemData(iter.index(), Column::ColumnStatus, FileStatus::Unreadable);
            else {
                dataMaintainer->updateChecksum(iter.index(), checksum);
                ++doneNum;
            }
        }
    }

    dataMaintainer->updateNumbers();

    if (finalProcess)
        emit processing(false); // set Mode view, hide progress bar

    return doneNum;
}

void Manager::showFileCheckResultMessage(bool isMatched, const QString &fileName)
{
    QString file_name;
    if (!fileName.isEmpty())
        file_name = "\n\n" + fileName;

    if (isMatched)
        emit showMessage(QString("Checksum Match%1").arg(file_name), "Success");
    else
        emit showMessage(QString("Checksum does NOT match%1").arg(file_name), "Failed");
}

// info about folder (number of files and total size) or file (size)
void Manager::getPathInfo(const QString &path)
{
    if (isViewFileSysytem) {
        QFileInfo fileInfo(path);

        // If a file path is specified, then there is no need to complicate this task and create an Object and a Thread
        // If a folder path is specified, then that folder should be iterated on a separate thread to be able to interrupt this process
        if (fileInfo.isFile()) {
            emit setStatusbarText(format::fileNameAndSize(path));
        }
        else if (fileInfo.isDir()) {
            QThread *thread = new QThread;
            Files *files = new Files(path);
            files->moveToThread(thread);

            connect(this, &Manager::cancelProcess, files, &Files::cancelProcess, Qt::DirectConnection);
            connect(thread, &QThread::finished, thread, &QThread::deleteLater);
            connect(thread, &QThread::finished, files, &Files::deleteLater);
            connect(thread, &QThread::started, files, qOverload<>(&Files::contentStatus));
            connect(files, &Files::setStatusbarText, this, [=](const QString &text){if (text != "counting...") thread->quit();});
            connect(files, &Files::setStatusbarText, this, [=](const QString &text){if (!text.isEmpty()) emit setStatusbarText(text);});

            // ***debug***
            // connect(thread, &Files::destroyed, this, [=]{qDebug()<< "Manager::getItemInfo | &Files::destroyed" << path;});

            thread->start();
        }
    }
}

void Manager::getIndexInfo(const QModelIndex &curIndex)
{
    if (!isViewFileSysytem && dataMaintainer->data_)
        emit setStatusbarText(dataMaintainer->itemContentsInfo(curIndex));
}

void Manager::folderContentsByType(const QString &folderPath)
{
    if (isViewFileSysytem) {
        QString statusText(QString("Contents of <%1>").arg(paths::basicName(folderPath)));
        emit setStatusbarText(statusText + ": processing...");

        QThread *thread = new QThread;
        Files *files = new Files(folderPath);
        files->moveToThread(thread);

        connect(this, &Manager::cancelProcess, files, &Files::cancelProcess, Qt::DirectConnection);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        connect(thread, &QThread::finished, files, &Files::deleteLater);
        connect(thread, &QThread::started, files, qOverload<>(&Files::folderContentsByType));
        connect(files, &Files::sendText, this, [=](const QString &text){thread->quit(); if (!text.isEmpty())
                        {emit showMessage(text, statusText); emit setStatusbarText(QStringList(text.split("\n")).last());}});

        // ***debug***
        connect(thread, &QThread::destroyed, this, [=]{qDebug()<< "Manager::folderContentsByType | &QThread::destroyed" << folderPath;});

        thread->start();
    }
    else {
        qDebug() << "Manager::folderContentsByType | Not a filesystem view";
    }
}

void Manager::modelChanged(ModelView modelView)
{
    isViewFileSysytem = (modelView == ModelView::FileSystem);

    if (isViewFileSysytem) {
        dataMaintainer->clearData(); // if the View is switched to the filesystem, then main data is no longer needed
        emit setPermanentStatus();
    }
}
