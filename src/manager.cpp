#include "manager.h"
#include "files.h"
#include "QFile"
#include "QFileInfo"
#include "QDir"

Manager::Manager(QObject *parent)
    : QObject{parent}
{
    connections();
    qDebug()<<"Manager created. Thread:"<<QThread::currentThread();
}

Manager::~Manager()
{
    emit cancelProcess();

    delete shaCalc;
    delete json;

    deleteCurData();
    qDebug()<<"Manager DESTRUCTED. Thread:"<<QThread::currentThread();
}

void Manager::connections()
{
    connect(this, &Manager::cancelProcess, shaCalc, &ShaCalculator::cancelProcess);
    connect(shaCalc, &ShaCalculator::donePercents, this, &Manager::donePercents);
    connect(shaCalc, &ShaCalculator::status, this, &Manager::status);
    connect(json, &jsonDB::showMessage, this, &Manager::showMessage);
}

void Manager::processFolderSha(const QString &folderPath, const int &shatype)
{
    Files F (folderPath);
    DataContainer calcData (folderPath);
    QStringList fileList;

    if (settings.value("dbPrefix").isValid()) {
        calcData.setJsonFileNamePrefix(settings.value("dbPrefix").toString());
    }

    if (settings.value("onlyExtensions").isValid() && !settings.value("onlyExtensions").toStringList().isEmpty()) {
        calcData.setOnlyExtensions(settings.value("onlyExtensions").toStringList());
        fileList = F.filteredFileList(calcData.onlyExtensions, true); // the list with only those files whose extensions are specified
    }
    else {
        QStringList ignoreList;
        if (settings.value("ignoredExtensions").isValid() && !settings.value("ignoredExtensions").toStringList().isEmpty()) {
            calcData.setIgnoredExtensions(settings.value("ignoredExtensions").toStringList());
            ignoreList.append(settings.value("ignoredExtensions").toStringList());
        }

        // adding *.ver.json or *.shaX file types to ignore list, if needed
        if (settings.value("ignoreDbFiles").isValid() && settings.value("ignoreDbFiles").toBool())
            ignoreList.append("ver.json");
        if (settings.value("ignoreShaFiles").isValid() && settings.value("ignoreShaFiles").toBool())
            ignoreList.append({"sha1", "sha256", "sha512"});

        fileList = F.filteredFileList(ignoreList); // the list of all files except ignored
    }

    if (fileList.isEmpty()) {
        if (F.allFiles().isEmpty())
            emit showMessage("Empty folder. Nothing to do");
        else
            emit showMessage("All files have been excluded.\nFiltering rules can be changed in the settings.");
        return;
    }

    emit setMode("processing");

    calcData.mainData = shaCalc->calculateSha(fileList, shatype);

    if (calcData.mainData.isEmpty()) {
        return;
    }

    json->makeJson(&calcData, QString("SHA-%1 Checksums for %2 files calculated").arg(shatype).arg(fileList.size()));

    emit setMode("endProcess");
}

void Manager::processFileSha(const QString &filePath, const int &shatype)
{
    emit setMode("processing");

    QString sum = shaCalc->calculateSha(filePath, shatype);
    if (sum.isEmpty()) {
        return;
    }

    //emit toClipboard(sum); // send checksum to clipboard

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

    emit setMode("endProcess");
}

void Manager::makeTreeModel(const QMap<QString,QString> &map)
{
    if (!map.isEmpty()) {
        emit status("File tree creation...");
        TreeModel *model = new TreeModel;
        model->setObjectName("treeModel");
        model->populateMap(toRelativePathsMap(map, curData->workDir));

        emit setModel(model);
        emit workDirChanged(curData->workDir);
        emit status(QString("SHA-%1: %2 files").arg(curData->shaType()).arg(map.size()));
    }
    else {
        qDebug()<< "Manager::makeTreeModel | Empty model";
        emit setModel();
    }
}

void Manager::resetDatabase()
{
    makeJsonModel(curData->jsonFilePath);
}

//making tree model | file paths : info about current availability on disk
void Manager::makeJsonModel(const QString &jsonFilePath)
{
    if (!jsonFilePath.endsWith(".ver.json", Qt::CaseInsensitive)) {
        emit showMessage(QString("Wrong file: %1\nExpected file extension '*.ver.json'").arg(jsonFilePath), "Wrong DB file!");
        emit setModel();
        return;
    }

    DataContainer *newData = json->parseJson(jsonFilePath);
    if (newData == nullptr) {
        emit setModel();
        return;
    }

    DataContainer *oldData = curData;
    curData = newData;

    if (oldData != nullptr) {
        delete oldData;
    }

    emit status("Json Model creation...");
    makeTreeModel(curData->defineFilesAvailability());

    setMode_model();
    emit showMessage(curData->aboutDb(), "Database parsed");
}

void Manager::showNewLostOnly()
{
    makeTreeModel(curData->newlostOnly());
}

//remove lost files, add new files
void Manager::updateNewLost()
{
    emit setMode("processing");
    QMap<QString,QString> changes; // display list of added/removed files from database
    QString info = QString("Database updated. Added %1 files, removed %2").arg(curData->newFiles.size()).arg(curData->lostFiles.size());

    if (curData->newFiles.size() > 0) {
        QMap<QString,QString> newFilesSums = shaCalc->calculateSha(curData->newFiles, curData->shaType());
        if(newFilesSums.isEmpty()) {
            return;
        }
        changes.insert(curData->updateMainData(newFilesSums)); // add new data to Database, returns the list of changes
    }

    if (curData->lostFiles.size() > 0)
        changes.insert(curData->clearDataFromLostFiles()); // remove lostFiles items from mainData

    json->makeJson(curData, info);
    makeTreeModel(changes);

    emit setMode("endProcess");
    emit setMode("model");
}

// update json Database with new checksums for files with failed verification
void Manager::updateMismatch()
{
    QMap<QString,QString> changes;
    int number = curData->mismatches.size();

    if (number > 0) {
        // update data and make 'changes' map to display a filelist with updated checksums, original lists 'mismatches' and 'recalculated' will be cleared
        changes = curData->updateMainData(curData->mismatches, "stored checksum updated");
    }
    else {
        qDebug()<<"Manager::updateMismatch() | ZERO differences.size()";
        return;
    }

    json->makeJson(curData, QString("Chechsums updated for %1 files").arg(number));
    makeTreeModel(changes);
    setMode_model();
}

// checking the list of files against the checksums stored in the database
void Manager::verifyFileList()
{
    if (curData->mainData.isEmpty()) {
        qDebug() << "mainData is Empty";
        return;
    }

    emit setMode("processing");

    curData->recalculated = shaCalc->calculateSha(curData->onDiskFiles, curData->shaType());
    if (curData->recalculated.isEmpty()) {
        return;
    }

    QMapIterator<QString,QString> ii(curData->recalculated);

    while (ii.hasNext()) {
        ii.next();
        if (curData->mainData.value(ii.key()) != ii.value())
            curData->mismatches.insert(ii.key(), ii.value());
    }

    if (curData->mismatches.size() > 0) {
        makeTreeModel(curData->fillMapSameValues(curData->mismatches.keys(), "NOT match"));
        emit showMessage(QString("%1 files changed or corrupted").arg(curData->mismatches.size()),"FAILED");
        emit setMode("endProcess");
        emit setMode("updateMismatch");
        return;
    }
    else {
        emit showMessage(QString("ALL %1 files passed the verification.\nStored SHA-%2 chechsums matched.").arg(curData->recalculated.size()).arg(curData->shaType()), "Success");
    }

    emit setMode("endProcess");
}

void Manager::checkFileSummary(const QString &path)
{
    QFileInfo fileInfo (path);
    QString ext = fileInfo.suffix().toLower();
    int shatype = ext.remove("sha").toInt();
    QString checkFileName = fileInfo.completeBaseName();
    QString checkFilePath = Files::joinPath(Files::parentFolder(path), checkFileName);

    QFile sumFile(path);
    QString line;
    if (sumFile.open(QFile::ReadOnly))
        line = sumFile.readLine();
    else {
        emit showMessage("Error while reading Summary File", "Error");
        return;
    }

    if (!QFileInfo(checkFilePath).isFile()) {
        checkFilePath = line.mid(shaStrLen(shatype)+2).replace("\n","");
        if (!QFileInfo(checkFilePath).isFile()) {
            emit showMessage("No File to check", "Warning");
            return;
        }
    }

    emit setMode("processing");
    QString savedSum = line.mid(0, shaStrLen(shatype));
    QString sum = shaCalc->calculateSha(checkFilePath, shatype);
    if (sum.isEmpty()) {
        return;
    }

    if (sum == savedSum) {
        emit showMessage("Checksum Match", "Success");
    }
    else {
        emit showMessage("Checksum does NOT match", "Failed");
    }
    emit setMode("endProcess");
}

//check only selected file instead all database cheking
void Manager::checkCurrentItemSum(const QString &path)
{
    QString filepath = Files::joinPath(curData->workDir, path);
    qDebug()<<"Manager::checkCurrentItemSum | filepath:"<<filepath;

    if (QFileInfo(filepath).isDir()) {
        emit showMessage("Currently only files can be checked independently.\nPlease select a file or check the whole database", "Warning");
        return;
    }
    if (!QFileInfo(filepath).isFile()) {
        emit showMessage("File does not exist", "Warning");
        return;
    }

    if (!curData->mainData.contains(filepath)) {
        emit showMessage("Checksum is missing in the database.\nPlease Update New/Lost", "NEW File");
        return;
    }

    QString savedSum = curData->mainData.value(filepath);
    if (savedSum == "unreadable") {
        emit showMessage("This file has been excluded (Unreadable).\nNo checksum in the database.", "Excluded File");
        return;
    }

    emit setMode("processing");
    QString sum = shaCalc->calculateSha(filepath, curData->shaType());
    if (sum.isEmpty()) {
        return;
    }

    if (sum == savedSum) {
        emit showMessage("Checksum Match", "Success");
    }
    else {
        emit showMessage("Checksum does NOT match", "Failed");
    }
    emit setMode("endProcess");
}

// info about folder (number of files and total size) or file (size)
void Manager::getItemInfo(const QString &path)
{
    if (isViewFileSysytem) {
        emit cancelProcess();
        QFileInfo fileInfo(path);

        // If a file path is specified, then there is no need to complicate this task and create an Object and a Thread
        // If a folder path is specified, then that folder should be iterated on a separate thread to be able to interrupt this process
        if (fileInfo.isFile()) {
            emit status(Files::fileSize(path));
        }
        else if (fileInfo.isDir()) {
            QThread *thread = new QThread;
            Files *files = new Files(path);
            files->moveToThread(thread);

            connect(this, &Manager::cancelProcess, files, &Files::cancelProcess, Qt::DirectConnection);
            connect(thread, &QThread::finished, thread, &QThread::deleteLater);
            connect(thread, &QThread::finished, files, &Files::deleteLater);
            connect(thread, &QThread::started, files, qOverload<>(&Files::contentStatus));
            connect(files, &Files::sendText, this, [=](const QString &text){if (text != "counting...") thread->quit();});
            connect(files, &Files::sendText, this, [=](const QString &text){if (!text.isEmpty()) emit status(text);});

            // **** debug info, can be removed in release
            //connect(files, &Files::destroyed, this, [=]{qDebug()<< "Manager::getItemInfo | Files destroyed: " << path;});
            connect(thread, &QThread::destroyed, this, [=]{qDebug()<< "Manager::getItemInfo | Thread destroyed:" << path;});

            thread->start();
        }
    }
    else
        emit status(curData->itemContentsInfo(path));
}

void Manager::folderContentsByType(const QString &folderPath)
{
    if (isViewFileSysytem) {
        emit cancelProcess();

        QString statusText(QString("Contents of: %1").arg(Files::folderName(folderPath)));
        emit status(statusText);

        QThread *thread = new QThread;
        Files *files = new Files(folderPath);
        files->moveToThread(thread);

        connect(this, &Manager::cancelProcess, files, &Files::cancelProcess, Qt::DirectConnection);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        connect(thread, &QThread::finished, files, &Files::deleteLater);
        connect(thread, &QThread::started, files, qOverload<>(&Files::folderContentsByType));
        connect(files, &Files::sendText, this, [=](const QString &text){if (!text.isEmpty()) emit showMessage(text, statusText); thread->quit();});

        // **** debug info, can be removed in release
        //connect(files, &Files::destroyed, this, [=]{qDebug()<< "Manager::folderContentsByType | Files destroyed: " << folderPath;});
        connect(thread, &QThread::destroyed, this, [=]{qDebug()<< "Manager::folderContentsByType | Thread destroyed:" << folderPath;});

        thread->start();
    }
    else {
        qDebug()<< "Manager::folderContentsByType | Not a filesystem view";
    }
}

void Manager::isViewFS(const bool isFS)
{
    isViewFileSysytem = isFS;
    qDebug() << "Manager::isViewFS" << isViewFileSysytem;
}

//if there are New Files or Lost Files --> setMode("modelNewLost"); else setMode("model");
void Manager::setMode_model()
{
    if (curData->newFiles.size() > 0 || curData->lostFiles.size() > 0)
        emit setMode("modelNewLost");
    else {
        emit setMode("model");
    }
}

// converting paths from full to relative
QStringList Manager::toRelativePathsList (const QStringList &filelist, const QString &relativeFolder)
{
    QDir folder (relativeFolder);
    QStringList relativePaths;

    for (int var = 0; var < filelist.size(); ++var) {
        relativePaths.append(folder.relativeFilePath(filelist.at(var)));
    }

    return relativePaths;
}

// converting paths (keys) from full to relative
QMap<QString,QString> Manager::toRelativePathsMap (const QMap<QString,QString> &filesMap, const QString &relativeFolder)
{
    QDir folder (relativeFolder);
    QMap<QString,QString> relativePaths;

    QMapIterator<QString,QString> i (filesMap);
    while (i.hasNext()) {
        i.next();
        relativePaths.insert(folder.relativeFilePath(i.key()), i.value());
    }

    return relativePaths;
}

int Manager::shaStrLen(const int &shatype)
{
    if (shatype == 1)
        return 40;
    else if (shatype == 256)
        return 64;
    else if (shatype == 512)
        return 128;
    else {
        qDebug()<<"Manager::shaStrLen | Wrong input shatype:"<<shatype;
        return 0;
    }
}

void Manager::getSettings(const QVariantMap &settingsMap)
{
    settings = settingsMap;
    qDebug() << "Manager::getSettings | " << settings;
}

void Manager::deleteCurData()
{
    /*if (curData != nullptr) {
        delete curData;
        curData = nullptr;
    }*/
    DataContainer *oldData = curData;
    curData = nullptr;
    if (oldData != nullptr) {
        delete oldData;
    }
}
