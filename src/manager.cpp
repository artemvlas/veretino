#include "manager.h"
#include "files.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>

Manager::Manager(QObject *parent)
    : QObject(parent)
{
    connections();
}

Manager::~Manager()
{
    delete shaCalc;
    deleteCurData();
}

void Manager::connections()
{
    connect(this, &Manager::cancelProcess, shaCalc, &ShaCalculator::cancelProcess);
    connect(shaCalc, &ShaCalculator::donePercents, this, &Manager::donePercents);
    connect(shaCalc, &ShaCalculator::setStatusbarText, this, &Manager::setStatusbarText);
}

void Manager::processFolderSha(const QString &folderPath, int shatype)
{
    emit setMode(Mode::Processing);

    Files F(folderPath);
    connect(this, &Manager::cancelProcess, &F, &Files::cancelProcess, Qt::DirectConnection);
    connect(&F, &Files::setStatusbarText, this, &Manager::setStatusbarText);

    DataContainer calcData;
    calcData.metaData.workDir = folderPath;
    calcData.metaData.shaType = shatype;

    // database filename
    QString dbPrefix;
    if (settings.value("dbPrefix").isValid())
        dbPrefix = settings.value("dbPrefix").toString();
    else
        dbPrefix = "checksums";

    calcData.metaData.databaseFileName = QString("%1_%2.ver.json").arg(dbPrefix, paths::folderName(folderPath));

    // filter rule
    if (settings.value("onlyExtensions").isValid() && !settings.value("onlyExtensions").toStringList().isEmpty()) {
        calcData.metaData.filter.extensionsList = settings.value("onlyExtensions").toStringList();
    }
    else {
        QStringList ignoreList;
        if (settings.value("ignoredExtensions").isValid() && !settings.value("ignoredExtensions").toStringList().isEmpty()) {
            ignoreList.append(settings.value("ignoredExtensions").toStringList());
        }

        // adding *.ver.json or *.shaX file types to ignore list, if needed
        if (settings.value("ignoreDbFiles").isValid() && settings.value("ignoreDbFiles").toBool())
            ignoreList.append("ver.json");
        if (settings.value("ignoreShaFiles").isValid() && settings.value("ignoreShaFiles").toBool())
            ignoreList.append({"sha1", "sha256", "sha512"});

        calcData.metaData.filter.include = false;
        calcData.metaData.filter.extensionsList = ignoreList;
    }

    // create the filelist
    calcData.filesData = F.allFiles(calcData.metaData.filter);

    // clear unneeded filtering rules
    calcData.metaData.filter.extensionsList.removeOne("ver.json");
    calcData.metaData.filter.extensionsList.removeOne("sha1");
    calcData.metaData.filter.extensionsList.removeOne("sha256");
    calcData.metaData.filter.extensionsList.removeOne("sha512");

    // exception and cancelation handling
    if (calcData.filesData.isEmpty()) {
        if (!F.canceled) {
            if (!calcData.metaData.filter.extensionsList.isEmpty())
                emit showMessage("All files have been excluded.\nFiltering rules can be changed in the settings.");
            else
                emit showMessage("Empty folder. Nothing to do");
        }
        else {
            emit setStatusbarText("Canceled");
        }
        emit setMode(Mode::EndProcess);
        return;
    }

    QString permStatus = QString("SHA-%1").arg(shatype);
    if (!calcData.metaData.filter.extensionsList.isEmpty())
        permStatus.prepend("filters applied | ");

    emit setPermanentStatus(permStatus);

    // calculating checksums
    calcData.filesData = shaCalc->calculate(calcData);

    emit setPermanentStatus();

    if (calcData.filesData.isEmpty()) {
        return;
    }

    // saving to json
    calcData.metaData.about = QString("SHA-%1 Checksums for %2 files calculated").arg(shatype).arg(calcData.filesData.size());

    curData = new DataMaintainer(calcData);
    connect(curData, &DataMaintainer::showMessage, this, &Manager::showMessage);
    connect(curData, &DataMaintainer::setStatusbarText, this, &Manager::setStatusbarText);
    curData->exportToJson();

    deleteCurData();

    emit setMode(Mode::EndProcess);
}

void Manager::processFileSha(const QString &filePath, int shatype, bool summaryFile, bool clipboard)
{
    emit setMode(Mode::Processing);

    QString sum = shaCalc->calculate(filePath, shatype);
    if (sum.isEmpty()) {
        return;
    }

    if (clipboard) {
        emit toClipboard(sum); // send checksum to clipboard
        emit setStatusbarText("Computed checksum copied to clipboard");
    }

    if (summaryFile) {
        QString summaryFile = QString("%1.sha%2").arg(filePath).arg(shatype);
        QFile file(summaryFile);
        if (file.open(QFile::WriteOnly)) {
            file.write(QString("%1 *%2").arg(sum, QFileInfo(filePath).fileName()).toUtf8());
            emit showMessage(QString("Checksum saved to summary file:\n%1").arg(QFileInfo(summaryFile).fileName()));
        }
        else {
            emit toClipboard(sum); // send checksum to clipboard
            emit showMessage(QString("Unable to write to file: %1\nChecksum copied to clipboard").arg(summaryFile), "Warning");
        }
    }

    emit setMode(Mode::EndProcess);
}

void Manager::makeTreeModel(const FileList &data)
{
    if (!data.isEmpty()) {
        emit setStatusbarText("File tree creation...");
        TreeModel *model = new TreeModel;
        model->setObjectName("treeModel");
        model->populate(data);

        emit setModel(model);
        emit workDirChanged(curData->data_.metaData.workDir);
        /*emit setStatusbarText(QString("SHA-%1: %2 files")
                 .arg(curData->data_.metaData.shaType)
                 .arg(curData->data_.filesData.size()));*/
    }
    else {
        qDebug() << "Manager::makeTreeModel | Empty model";
        emit setModel();
    }
}

void Manager::resetDatabase()
{
    QString dbFilePath;
    if (curData->data_.metaData.databaseFileName.contains('/'))
        dbFilePath = curData->data_.metaData.databaseFileName;
    else
        dbFilePath = paths::joinPath(curData->data_.metaData.workDir, curData->data_.metaData.databaseFileName);

    createDataModel(dbFilePath);
}

//making tree model | file paths : info about current availability on disk
void Manager::createDataModel(const QString &databaseFilePath)
{
    if (!tools::isDatabaseFile(databaseFilePath)) {
        emit showMessage(QString("Wrong file: %1\nExpected file extension '*.ver.json'").arg(databaseFilePath), "Wrong DB file!");
        emit setModel();
        return;
    }

    DataMaintainer *oldData = curData;

    curData = new DataMaintainer;
    connect(this, &Manager::cancelProcess, curData, &DataMaintainer::cancelProcess, Qt::DirectConnection);
    connect(curData, &DataMaintainer::showMessage, this, &Manager::showMessage);
    connect(curData, &DataMaintainer::setStatusbarText, this, &Manager::setStatusbarText);
    connect(curData, &DataMaintainer::setPermanentStatus, this, &Manager::setPermanentStatus);
    connect(curData, &DataMaintainer::itemStatusChanged, this, &Manager::setItemStatus);

    curData->importJson(databaseFilePath);

    if (curData->data_.filesData.isEmpty()) {
        delete curData;
        curData = oldData;
        qDebug() << "Manager::createDataModel | DataMaintainer 'oldData' restored" << databaseFilePath;

        emit setModel();
        return;
    }

    if (oldData != nullptr) {
        delete oldData;
    }

    dbStatus();
    makeTreeModel(curData->data_.filesData);
    chooseMode();
}

void Manager::showNewLostOnly()
{
    makeTreeModel(curData->listOf({FileValues::New, FileValues::Missing}));
}

void Manager::updateNewLost()
{
    QString itemsInfo;

    if (curData->data_.metaData.numNewFiles > 0) {
        DataContainer dataCont(curData->data_.metaData);
        dataCont.filesData =  curData->listOf(FileValues::New);

        emit setMode(Mode::Processing);
        if (curData->updateData(shaCalc->calculate(dataCont), FileValues::Added) == 0)
            return;
        emit setMode(Mode::EndProcess);

        itemsInfo = QString("added %1").arg(curData->data_.metaData.numNewFiles);
        if (curData->data_.metaData.numMissingFiles > 0)
            itemsInfo.append(", ");
    }

    if (curData->data_.metaData.numMissingFiles > 0) {
        itemsInfo.append(QString("removed %1").arg(curData->clearDataFromLostFiles()));
    }

    curData->data_.metaData.about = QString("Database updated: %1 items").arg(itemsInfo);

    curData->exportToJson();

    emit setMode(Mode::Model);
}

// update the Database with new checksums for files with failed verification
void Manager::updateMismatch()
{
    curData->data_.metaData.about = QString("%1 checksums updated")
                        .arg(curData->updateMismatchedChecksums());

    curData->exportToJson();
    chooseMode();
}

// checking the list of files against stored in the database checksums
void Manager::verifyFileList(const QString &subFolder)
{
    FileList fileList;

    if (!subFolder.isEmpty())
        fileList = curData->subfolderContent(subFolder);
    else
        fileList = curData->data_.filesData;

    if (fileList.isEmpty()) {
        qDebug() << "Manager::verifyFileList --> fileList is Empty";
        return;
    }

    emit setMode(Mode::Processing);
    DataContainer dataCont(curData->data_.metaData);
    dataCont.filesData = curData->listOf(FileValues::NotChecked, fileList);
    FileList recalculated = shaCalc->calculate(dataCont);
    emit setMode(Mode::EndProcess);

    if (!recalculated.isEmpty()) {
        FileList::const_iterator iter;
        for (iter = recalculated.constBegin(); iter != recalculated.constEnd(); ++iter) {
            curData->updateData(iter.key(), iter.value().checksum);
        }

        if (fileList.size() == curData->data_.filesData.size())
            curData->data_.metaData.isChecked = true;
    }

    if (!recalculated.isEmpty() || dataCont.filesData.isEmpty()) {
        curData->updateMetaData();
        if (curData->data_.metaData.numMismatched > 0) {
            emit showMessage(QString("%1 files changed or corrupted").arg(curData->data_.metaData.numMismatched), "FAILED");
            emit setMode(Mode::UpdateMismatch);
            makeTreeModel(curData->listOf(FileValues::Mismatched));
        }
        else
            emit showMessage(QString("ALL %1 files passed the verification.\nStored SHA-%2 chechsums matched.")
                                 .arg(curData->data_.metaData.numMatched).arg(curData->data_.metaData.shaType), "Success");
    }
}

void Manager::checkFileSummary(const QString &path)
{
    QFileInfo fileInfo(path);
    QString ext = fileInfo.suffix().toLower();
    int shatype = ext.remove("sha").toInt();
    QString checkFileName = fileInfo.completeBaseName();
    QString checkFilePath = paths::joinPath(paths::parentFolder(path), checkFileName);

    QFile sumFile(path);
    QString line;
    if (sumFile.open(QFile::ReadOnly))
        line = sumFile.readLine();
    else {
        emit showMessage("Error while reading Summary File", "Error");
        return;
    }

    if (!QFileInfo(checkFilePath).isFile()) {
        checkFilePath = paths::joinPath(paths::parentFolder(path), line.mid(tools::shaStrLen(shatype) + 2).remove("\n"));
        if (!QFileInfo(checkFilePath).isFile()) {
            emit showMessage("No File to check", "Warning");
            return;
        }
    }

    emit setMode(Mode::Processing);   
    QString sum = shaCalc->calculate(checkFilePath, shatype);
    emit setMode(Mode::EndProcess);

    if (!sum.isEmpty())
        showFileCheckResultMessage(sum == line.mid(0, tools::shaStrLen(shatype)).toLower());
}

//check only selected file instead all database cheking
void Manager::checkCurrentItemSum(const QString &path)
{
    QString savedSum = copyStoredChecksum(path, false);

    if (!savedSum.isEmpty()) {
        emit setMode(Mode::Processing);
        QString sum = shaCalc->calculate(paths::joinPath(curData->data_.metaData.workDir, path),  curData->data_.metaData.shaType);
        emit setMode(Mode::EndProcess);

        if (!sum.isEmpty()) {
            showFileCheckResultMessage(curData->updateData(path, sum));
            curData->updateMetaData();
            chooseMode();
        }
    }
}

void Manager::showFileCheckResultMessage(bool isMatched)
{
    if (isMatched)
        emit showMessage("Checksum Match", "Success");
    else
        emit showMessage("Checksum does NOT match", "Failed");
}

QString Manager::copyStoredChecksum(const QString &path, bool clipboard)
{
    QString savedSum;

    if (curData->data_.filesData.value(path).status == FileValues::New)
        emit showMessage("The checksum is not yet in the database.\nPlease Update New/Lost", "NEW File");
    else if (curData->data_.filesData.value(path).status == FileValues::Unreadable)
        emit showMessage("This file has been excluded (Unreadable).\nNo checksum in the database.", "Excluded File");
    else {
        savedSum = curData->data_.filesData.value(path).checksum;
        if (savedSum.isEmpty())
            emit showMessage("No checksum in the database.", "No checksum");
        else if (clipboard)
            emit toClipboard(savedSum); // send checksum to clipboard
    }

    return savedSum;
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
    else
        emit setStatusbarText(curData->itemContentsInfo(path));
}

void Manager::folderContentsByType(const QString &folderPath)
{
    if (isViewFileSysytem) {
        QString statusText(QString("Contents of <%1>").arg(paths::folderName(folderPath)));
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
void Manager::showAll()
{
    makeTreeModel(curData->data_.filesData);
}

void Manager::dbStatus()
{
    curData->dbStatus();
}

void Manager::isViewFS(const bool isFS)
{
    isViewFileSysytem = isFS;
    qDebug() << "Manager::isViewFS" << isViewFileSysytem;
}

//if there are New Files or Lost Files --> setMode("modelNewLost"); else setMode("model");
void Manager::chooseMode()
{
    if (curData->data_.metaData.numMismatched > 0)
        emit setMode(Mode::UpdateMismatch);
    else if (curData->data_.metaData.numNewFiles > 0 || curData->data_.metaData.numMissingFiles > 0)
        emit setMode(Mode::ModelNewLost);
    else {
        emit setMode(Mode::Model);
    }
}

void Manager::getSettings(const QVariantMap &settingsMap)
{
    settings = settingsMap;
    qDebug() << "Manager::getSettings | " << settings;
}

void Manager::deleteCurData()
{
    if (curData != nullptr) {
        curData->deleteLater();
        curData = nullptr;
    }
}
