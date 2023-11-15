// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "manager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QThread>
#include <QDebug>
#include "files.h"
#include "shacalculator.h"
#include "procstate.h"
#include "treemodeliterator.h"

Manager::Manager(Settings *settings, QObject *parent)
    : QObject(parent), settings_(settings)
{
    connect(this, &Manager::cancelProcess, this, [=]{canceled = true; emit setStatusbarText("Canceled");}, Qt::DirectConnection);
}

Manager::~Manager()
{
    deleteCurData();
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

    emit setMode(Mode::Processing);

    curData = new DataMaintainer;
    connect(this, &Manager::cancelProcess, curData, &DataMaintainer::cancelProcess, Qt::DirectConnection);
    connect(curData, &DataMaintainer::showMessage, this, &Manager::showMessage);
    connect(curData, &DataMaintainer::setStatusbarText, this, &Manager::setStatusbarText);

    curData->data_->metaData.workDir = folderPath;
    curData->data_->metaData.algorithm = algo;
    curData->data_->metaData.filter = settings_->filter;
    curData->data_->metaData.databaseFilePath = paths::joinPath(folderPath, QString("%1_%2.ver.json").arg(settings_->dbPrefix, paths::basicName(folderPath)));

    // create the filelist
    curData->addActualFiles(Files::Queued);

    // exception and cancelation handling
    if (!curData->data_) {
        if (curData->canceled)
            emit setStatusbarText("Canceled");

        emit setMode(Mode::EndProcess);
        emit setModel();
        return;
    }

    emit setModel(curData->data_->proxyModel_);
    chooseMode();

    QString permStatus = format::algoToStr(algo);
    if (curData->data_->isFilterApplied())
        permStatus.prepend("filters applied | ");

    emit setPermanentStatus(permStatus);

    // calculating checksums
    int doneNumber = calculateChecksums(curData->data_);

    if (canceled || doneNumber == 0) {
        emit setMode(Mode::EndProcess);
        emit setModel();
        return;
    }

    // saving to json
    curData->data_->metaData.about = QString("%1 Checksums for %2 files calculated")
                                                    .arg(format::algoToStr(algo))
                                                    .arg(doneNumber);

    curData->exportToJson();

    //emit setPermanentStatus();
    //curData->updateNumbers();

    emit setMode(Mode::EndProcess);
    emit setMode(Mode::Model);
}

void Manager::processFileSha(const QString &filePath, QCryptographicHash::Algorithm algo, bool summaryFile, bool clipboard)
{
    QString sum = calculateChecksum(filePath, algo);
    if (sum.isEmpty())
        return;

    if (clipboard) {
        emit toClipboard(sum); // send checksum to clipboard
        emit setStatusbarText("Computed checksum is copied to clipboard");
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
            emit showMessage(QString("The checksum is saved in the summary file:\n%1").arg(paths::basicName(sumFile)));
        }
        else {
            emit toClipboard(sum); // send checksum to clipboard
            emit showMessage(QString("Unable to write to file: %1\nChecksum copied to clipboard").arg(sumFile), "Warning");
        }
    }
}

void Manager::resetDatabase()
{
    if (!curData)
        return;

    createDataModel(curData->data_->metaData.databaseFilePath);
}

void Manager::restoreDatabase()
{
    QString databaseFilePath;
    if (curData)
        databaseFilePath = curData->data_->metaData.databaseFilePath;
    if (restoreBackup(databaseFilePath))
        createDataModel(databaseFilePath);
    else
        emit setStatusbarText("No saved changes");
}

bool Manager::restoreBackup(const QString &databaseFilePath)
{
    QString backupFilePath = paths::backupFilePath(databaseFilePath);

    if (QFile::exists(backupFilePath)) {
        if (QFile::exists(databaseFilePath)) {
            if (!QFile::remove(databaseFilePath))
                return false;
        }

        return QFile::rename(backupFilePath, databaseFilePath);
    }
    return false;
}

//making tree model | file paths : info about current availability on disk
void Manager::createDataModel(const QString &databaseFilePath)
{
    if (!tools::isDatabaseFile(databaseFilePath)) {
        emit showMessage(QString("Wrong file: %1\nExpected file extension '*.ver.json'").arg(databaseFilePath), "Wrong DB file!");
        emit setModel();
        return;
    }

    oldData = curData;

    curData = new DataMaintainer;
    connect(this, &Manager::cancelProcess, curData, &DataMaintainer::cancelProcess, Qt::DirectConnection);
    connect(curData, &DataMaintainer::showMessage, this, &Manager::showMessage);
    connect(curData, &DataMaintainer::setStatusbarText, this, &Manager::setStatusbarText);
    connect(curData, &DataMaintainer::setPermanentStatus, this, &Manager::setPermanentStatus);

    curData->importJson(databaseFilePath);

    if (!curData->data_ || curData->data_->model_->isEmpty()) {
        delete curData;
        curData = oldData;
        qDebug() << "Manager::createDataModel | DataMaintainer 'oldData' restored" << databaseFilePath;

        emit setModel();
        return;
    }

    emit setModel(curData->data_->proxyModel_);

    dbStatus();
    chooseMode();
}

void Manager::updateNewLost()
{
    if (!curData)
        return;

    QString itemsInfo;

    if (curData->data_->numbers.numNewFiles > 0) {
        //emit setMode(Mode::Processing);
        curData->changeFilesStatuses(Files::New, Files::Queued);
        if (calculateChecksums(curData->data_) == 0)
            return;

        //emit setMode(Mode::EndProcess);

        itemsInfo = QString("added %1").arg(curData->data_->numbers.numNewFiles);
        if (curData->data_->numbers.numMissingFiles > 0)
            itemsInfo.append(", ");
    }

    if (curData->data_->numbers.numMissingFiles > 0) {
        if (curData->data_->numbers.numMissingFiles < (curData->data_->numbers.numChecksums + curData->data_->numbers.numNewFiles))
            itemsInfo.append(QString("removed %1").arg(curData->clearDataFromLostFiles()));
        else {
            emit showMessage("Failure to delete all database items", "Warning");
            return;
        }
    }

    curData->data_->metaData.about = QString("Database updated: %1 items").arg(itemsInfo);

    curData->exportToJson();

    emit setMode(Mode::Model);
    emit showFiltered();
}

// update the Database with new checksums for files with failed verification
void Manager::updateMismatch()
{
    if (!curData || !curData->data_)
        return;

    emit showFiltered();

    curData->data_->metaData.about = QString("%1 checksums updated")
                        .arg(curData->updateMismatchedChecksums());

    curData->exportToJson();
    chooseMode();
}

void Manager::verify(const QModelIndex &curIndex)
{
    ModelKit::isFileRow(curIndex) ? verifyFileItem(curIndex)
                                  : verifyFolderItem(curData->sourceIndex(curIndex));
}

//check only selected file instead all database cheking
void Manager::verifyFileItem(const QModelIndex &fileItemIndex)
{
    if (!curData)
        return;

    QString savedSum = curData->getStoredChecksum(fileItemIndex);

    if (!savedSum.isEmpty()) {
        QString sum = calculateChecksum(paths::joinPath(curData->data_->metaData.workDir, ModelKit::getPath(fileItemIndex)),  curData->data_->metaData.algorithm);

        if (!sum.isEmpty()) {
            showFileCheckResultMessage(curData->updateChecksum(fileItemIndex, sum));
            curData->updateNumbers();
            chooseMode();
        }
    }
}

void Manager::verifyFolderItem(const QModelIndex &folderItemIndex)
{
    if (!curData)
        return;

    curData->changeFilesStatuses(Files::NotChecked, Files::Queued, folderItemIndex);
    calculateChecksums(curData->data_, folderItemIndex);

    if (canceled)
        return;

    curData->updateNumbers();

    // result
    if (!folderItemIndex.isValid()) { // if root folder
        int numMatched = curData->data_->numbers.numMatched + curData->data_->numbers.numAdded + curData->data_->numbers.numChecksumUpdated;
        if (curData->data_->numbers.numMismatched > 0)
            emit showMessage(QString("%1 out of %2 files is changed or corrupted")
                                 .arg(curData->data_->numbers.numMismatched)
                                 .arg(numMatched), "FAILED");
        else if (numMatched > 0)
            emit showMessage(QString("ALL %1 files passed the verification.\nStored %2 checksums matched.")
                                 .arg(numMatched)
                                 .arg(format::algoToStr(curData->data_->metaData.algorithm)), "Success");
    }
    else {// if subfolder
        Numbers num = curData->updateNumbers(curData->data_->model_, folderItemIndex);
        int subMatched = num.numMatched + num.numAdded + num.numChecksumUpdated;

        if (num.numMismatched > 0)
            emit showMessage(QString("Subfolder: %1\n\n%2 out of %3 files in the Subfolder is changed or corrupted")
                                 .arg("subFolder").arg(num.numMismatched).arg(subMatched + num.numMismatched), "FAILED");
        else
            emit showMessage(QString("Subfolder: %1\n\nAll %2 checked files passed the verification")
                                 .arg("subFolder").arg(subMatched), "Success");
    }

    if (curData->data_->numbers.numMismatched > 0) {
        emit setMode(Mode::UpdateMismatch);
        emit showFiltered({FileStatus::Mismatched});
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
        showFileCheckResultMessage(sum == checkSum.toLower());
}

QString Manager::calculateChecksum(const QString &filePath, QCryptographicHash::Algorithm algo)
{
    ProcState state(QFileInfo(filePath).size());
    ShaCalculator shaCalc(algo);

    connect(this, &Manager::cancelProcess, &shaCalc, &ShaCalculator::cancelProcess, Qt::DirectConnection);
    connect(&shaCalc, &ShaCalculator::doneChunk, &state, &ProcState::doneChunk);
    connect(&state, &ProcState::donePercents, this, &Manager::donePercents);
    connect(&state, &ProcState::procStatus, this, &Manager::procStatus);

    emit setStatusbarText(QString("Calculating %1 checksum: %2").arg(format::algoToStr(algo), format::fileNameAndSize(filePath)));

    emit setMode(Mode::Processing);
    QString checkSum = shaCalc.calculate(filePath, algo);
    emit setMode(Mode::EndProcess);

    if (!checkSum.isEmpty())
        emit setStatusbarText(QString("%1 calculated").arg(format::algoToStr(algo)));
    else if (!canceled)
        emit setStatusbarText("read error");

    if (canceled)
        canceled = false;

    return checkSum;
}

int Manager::calculateChecksums(DataContainer* container, QModelIndex rootIndex)
{
    if (container->model_->isEmpty())
        return 0;

    if (rootIndex.isValid() && rootIndex.model() == container->proxyModel_)
        rootIndex = container->proxyModel_->mapToSource(rootIndex);

    qint64 totalSize = DataMaintainer::totalSizeOfListedFiles(container->model_, {Files::Queued}, rootIndex);
    QString totalSizeReadable = format::dataSizeReadable(totalSize);
    ProcState state(totalSize);
    ShaCalculator shaCalc(container->metaData.algorithm);
    canceled = false;
    int doneNum = 0;

    connect(this, &Manager::cancelProcess, &shaCalc, &ShaCalculator::cancelProcess, Qt::DirectConnection);
    connect(&shaCalc, &ShaCalculator::doneChunk, &state, &ProcState::doneChunk);
    connect(&state, &ProcState::donePercents, this, &Manager::donePercents);
    connect(&state, &ProcState::procStatus, this, &Manager::procStatus);

    TreeModelIterator iter(container->model_, rootIndex);

    emit setMode(Mode::Processing);
    while (iter.hasNext() && !canceled) {
        QString doneData;
        if (state.doneSize_ == 0)
            doneData = QString("(%1)").arg(totalSizeReadable);
        else
            doneData = QString("(%1 / %2)").arg(format::dataSizeReadable(state.doneSize_), totalSizeReadable);

        emit setStatusbarText(QString("Calculating %1 of %2 checksums %3")
                                  .arg(++doneNum)
                                  .arg(container->numbers.numQueued)
                                  .arg(doneData));

        iter.nextFile();
        if (iter.status() == Files::Queued) {
            container->model_->setItemData(iter.index(), ModelKit::ColumnStatus, Files::Processing);

            QString checksum = shaCalc.calculate(paths::joinPath(container->metaData.workDir, iter.path()), container->metaData.algorithm);
            if (checksum.isEmpty())
                container->model_->setItemData(iter.index(), ModelKit::ColumnStatus, Files::Unreadable);
            else
                curData->updateChecksum(iter.index(), checksum);
        }
    }
    emit setMode(Mode::EndProcess);

    if (canceled) {
        qDebug() << "Manager::calculateChecksums | Canceled";
        canceled = false;
        return 0;
    }

    emit setStatusbarText("Done");
    return doneNum;
}

void Manager::showFileCheckResultMessage(bool isMatched)
{
    if (isMatched)
        emit showMessage("Checksum Match", "Success");
    else
        emit showMessage("Checksum does NOT match", "Failed");
}

void Manager::copyStoredChecksum(const QModelIndex &fileItemIndex)
{
    if (!curData)
        return;

    emit toClipboard(curData->getStoredChecksum(fileItemIndex));
}

// info about folder (number of files and total size) or file (size)
void Manager::getItemInfo(const QString &path)
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
    else if (curData)
        emit setStatusbarText(curData->itemContentsInfo(ModelKit::getIndex(path, curData->data_->model_)));
        //emit setStatusbarText(curData->itemContentsInfo(path));
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
        qDebug()<< "Manager::folderContentsByType | Not a filesystem view";
    }
}

void Manager::dbStatus()
{
    if (curData)
        curData->dbStatus();
}

void Manager::modelChanged(const bool isFileSystem)
{
    isViewFileSysytem = isFileSystem;
    if (isFileSystem)
        deleteCurData(); // if the View is switched to the filesystem, then curData is no longer needed

    deleteOldData(); // delete backup of old curData if any
}

void Manager::chooseMode()
{
    if (!curData)
        return;

    if (curData->data_->numbers.numMismatched > 0)
        emit setMode(Mode::UpdateMismatch);
    else if (curData->data_->numbers.numNewFiles > 0 || curData->data_->numbers.numMissingFiles > 0)
        emit setMode(Mode::ModelNewLost);
    else {
        emit setMode(Mode::Model);
    }
}

void Manager::deleteCurData()
{
    if (curData) {
        qDebug() << "Manager::deleteCurData()";
        delete curData;
        curData = nullptr;
    }
}

void Manager::deleteOldData()
{
    if (oldData) {
        qDebug() << "Manager::deleteOldData()";
        delete oldData;
        oldData = nullptr;
    }
}
