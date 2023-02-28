#include "manager.h"
#include "QThread"

Manager::Manager(QObject *parent)
    : QObject{parent}
{
    connections();
    qDebug()<<"Manager created. Thread:"<<QThread::currentThread();
}

Manager::~Manager()
{
    delete shaCalc;
    delete json;
    deleteCurData();
    qDebug()<<"Manager DESTRUCTED. Thread:"<<QThread::currentThread();
}

void Manager::connections()
{
    connect(shaCalc,SIGNAL(donePercents(int)),this,SIGNAL(donePercents(int)));
    connect(shaCalc,SIGNAL(status(QString)),this,SIGNAL(status(QString)));

    connect(this, &Manager::cancelProcess, shaCalc, &ShaCalculator::cancelProcess, Qt::DirectConnection);

    connect(json,SIGNAL(status(QString)),this,SIGNAL(status(QString)));
    connect(json,SIGNAL(showMessage(QString,QString)),this,SIGNAL(showMessage(QString,QString)));
}

void Manager::processFolderSha(const QString &folderPath, const int &shatype)
{
    Files F (folderPath);
    DataContainer calcData (folderPath);
    QStringList fileList;

    if (settings["dbPrefix"].isValid()) {
        calcData.setJsonFileNamePrefix(settings["dbPrefix"].toString());
    }

    if (settings["onlyExtensions"].isValid() && !settings["onlyExtensions"].toStringList().isEmpty()) {
        calcData.setOnlyExtensions(settings["onlyExtensions"].toStringList());
        fileList = F.includedOnlyFilelist(calcData.onlyExtensions);
    }
    else {
        // to disable ignoring db (*.ver.json) or *.shaX files: F.ignoreDbFiles = false; F.ignoreShaFiles = false; to enable '= true' | 'true' is default in Files()
        if (settings["ignoreDbFiles"].isValid())
            F.ignoreDbFiles = settings["ignoreDbFiles"].toBool();
        if (settings["ignoreShaFiles"].isValid())
            F.ignoreShaFiles = settings["ignoreShaFiles"].toBool();

        if (settings["ignoredExtensions"].isValid() && !settings["ignoredExtensions"].toStringList().isEmpty()) {
            calcData.setIgnoredExtensions(settings["ignoredExtensions"].toStringList());
        }

        fileList = F.actualFileListFiltered(calcData.ignoredExtensions); //list of files except ignored
    }

    if(fileList.isEmpty()) {
        QString info;
        if (F.actualFiles.isEmpty())
            info = "Empty folder. Nothing to do";
        else
            info = "All files have been excluded.\nFiltering rules can be changed in the settings.";

        emit showMessage(info);
        return;
    }

    emit setMode("processing");

    calcData.mainData = shaCalc->calcShaList(fileList, shatype);

    if(calcData.mainData.isEmpty()) {
        return;
    }

    if (json->makeJsonDB(&calcData))
        emit showMessage(QString("SHA-%1 Checksums for %2 files calculated\nDatabase: %3\nuse it to check the data integrity").arg(shatype)
                         .arg(fileList.size()).arg(QFileInfo(calcData.jsonFilePath).fileName()), "Success");

    emit setMode("endProcess");
}

void Manager::processFileSha(const QString &filePath, const int &shatype)
{
    emit setMode("processing");

    QString sum = shaCalc->calcShaFile(filePath, shatype);
    if(sum == nullptr) {
        return;
    }

    //emit toClipboard(sum); // send checksum to clipboard

    QString summaryFile = QString("%1.sha%2").arg(filePath).arg(shatype);
    QFile file(summaryFile);
    if(file.open(QFile::WriteOnly)) {
        file.write(QString("%1 *%2").arg(sum, QFileInfo(filePath).fileName()).toUtf8());
        emit showMessage(QString("Checksum saved to summary file:\n%1").arg(QFileInfo(summaryFile).fileName()));
    }
    else {
        emit toClipboard(sum); // send checksum to clipboard
        emit showMessage(QString("Unable to write to file: %1\nChecksum copied to clipboard").arg(summaryFile), "Error");
    }

    emit setMode("endProcess");
}

void Manager::makeTreeModel(const QMap<QString,QString> &map)
{
    emit status("File tree creation...");

    if(!map.isEmpty()) {
        TreeModel *model = new TreeModel;
        model->setObjectName("treeModel");
        model->populateMap(toRelativePathsMap(map, curData->workDir));

        emit completeTreeModel(model);
        emit status(QString("SHA-%1: %2 files").arg(curData->dbShaType).arg(map.size()));
    }
    else
        emit resetView();
}

void Manager::resetDatabase()
{
    makeJsonModel(curData->jsonFilePath);
}

//making tree model | file paths : info about current availability on disk
void Manager::makeJsonModel(const QString &jsonFilePath)
{
    if(!jsonFilePath.endsWith(".ver.json")) {
        emit showMessage(QString("Wrong file: %1\nExpected file extension '*.ver.json'").arg(jsonFilePath), "Wrong DB file!");
        return;
    }

    DataContainer *oldData = curData;

    curData = json->parseJson(jsonFilePath);

    if (oldData != nullptr) {
        delete oldData;
    }

    if (curData == nullptr) {
        emit showMessage(QString("%1\n\nThe database doesn't contain checksums.\nProbably all files have been ignored.")
                         .arg(QFileInfo(jsonFilePath).fileName()), "Empty Database!");
        return;
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
    QString info = QString("Database updated. Added %1 files, removed %2").arg(curData->newFilesNumber).arg(curData->lostFilesNumber);

    if (curData->newFilesNumber > 0) {
        QMap<QString,QString> newFilesSums = shaCalc->calcShaList(curData->newFiles, curData->dbShaType);
        if(newFilesSums.isEmpty()) {
            return;
        }
        changes.insert(curData->updateMainData(newFilesSums)); // add new data to Database, returns the list of changes
    }

    if (curData->lostFilesNumber > 0)
        changes.insert(curData->clearDataFromLostFiles()); // remove lostFiles items from mainData

    if (json->makeJsonDB(curData)) {
        makeTreeModel(changes);
        emit showMessage(info);
    }

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

    if (json->makeJsonDB(curData)) {
        makeTreeModel(changes);
        setMode_model();
        emit showMessage(QString("Chechsums updated for %1 files").arg(number));
    }
}

// checking the list of files against the checksums stored in the database
void Manager::verifyFileList()
{
    if(curData->mainData.isEmpty()) {
        qDebug()<<"mainData is Empty";
        return;
    }

    emit setMode("processing");

    curData->recalculated = shaCalc->calcShaList(curData->onDiskFiles, curData->dbShaType);
    if(curData->recalculated.isEmpty()) {
        return;
    }

    QMapIterator<QString,QString> ii (curData->recalculated);

    while(ii.hasNext()) {
        ii.next();
        if (curData->mainData[ii.key()] != ii.value())
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
        emit showMessage(QString("ALL %1 files passed the verification.\nStored SHA-%2 chechsums matched.").arg(curData->recalculated.size()).arg(curData->dbShaType), "Success");
    }

    emit setMode("endProcess");
}

void Manager::checkFileSummary(const QString &path)
{
    QString ext = QFileInfo(path).suffix().toLower();
    int shatype = ext.replace("sha","").toInt();
    QString checkFileName = QFileInfo(path).completeBaseName();
    QString checkFilePath = QFileInfo(path).absolutePath() + "/" + checkFileName;

    QFile sumFile(path);
    QString line;
    if(sumFile.open(QFile::ReadOnly))
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
    QString sum = shaCalc->calcShaFile(checkFilePath, shatype);
    if(sum == nullptr) {
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
    QString filepath = curData->workDir + path;
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

    QString savedSum = curData->mainData[filepath];
    if (savedSum == "unreadable") {
        emit showMessage("This file has been excluded (Unreadable).\nNo checksum in the database.", "Excluded File");
        return;
    }

    emit setMode("processing");
    QString sum = shaCalc->calcShaFile(filepath, curData->dbShaType);
    if(sum == nullptr) {
        return;
    }

    if (sum == savedSum) {
        emit showMessage("Checksum Match", "Success");
    }
    else {
        emit showMessage("Checksum does NOT match", "Failed");
    }
    emit setMode("endProcess");
    //setMode_model();
}

// info about folder (number of files and total size) or file (size)
void Manager::getItemInfo(const QString &path)
{
    if (isViewFileSysytem) {
        emit status("Counting...");

        QString text;
        QFileInfo f (path);
        if(f.isDir())
             text = Files(path).folderContentStatus();
        else if(f.isFile())
            text = Files(path).fileNameSize();

        emit status(text);
    }
    else
        emit status(curData->itemContentsInfo(path));
}

void Manager::isViewFS(const bool isFS)
{
    isViewFileSysytem = isFS;
    qDebug()<<"Manager::isViewFS"<<isViewFileSysytem;
}

//if there are New Files or Lost Files --> setMode("modelNewLost"); else setMode("model");
void Manager::setMode_model()
{
    if (curData->newFiles.size() > 0 || curData->lostFiles.size() > 0) {
        //emit status(QString("SHA-%1: %2 new files, %3 lost files | Total files listed: %4").arg(curData->dbShaType).arg(curData->newFiles.size()).arg(curData->lostFiles.size()).arg(curData->filesAvailability.size()));
        emit setMode("modelNewLost");
    }
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
