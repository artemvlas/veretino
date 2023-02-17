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
    //std::auto_ptr<ShaCalculator> shaCalc (new ShaCalculator);

    Files F;
    // to disable filtering db (*.ver.json) or *.shaX files: F.filterDbFiles = false; F.filterShaFiles = false; to enable '= true' | 'true' is default in Files()
    if (settings["filterDbFiles"].isValid())
        F.filterDbFiles = settings["filterDbFiles"].toBool();
    if (settings["filterShaFiles"].isValid())
        F.filterShaFiles = settings["filterShaFiles"].toBool();

    if (settings["extensions"].isValid() && !settings["extensions"].isNull()) {
        json->filteredExtensions = settings["extensions"].toStringList();
    }

    QStringList fileList = F.actualFileListFiltered(folderPath, json->filteredExtensions); //list of files except filtered

    if(fileList.isEmpty()) {
        emit showMessage("Empty folder. Nothing to do");
        return;
    }

    emit setMode("processing");
    QMap<QString,QString> calculatedMap = shaCalc->calcShaList(fileList, shatype);

    if(calculatedMap.isEmpty()) {
        return;
    }

    QString filePath = QString("%1/checksums_%2.ver.json").arg(folderPath, QDir(folderPath).dirName());

    if (json->makeJsonDB(calculatedMap, filePath))
        emit showMessage(QString("SHA-%1 Checksums for %2 files calclutated\nDatabase: %3").arg(shatype).arg(fileList.size()).arg(QFileInfo(filePath).fileName()), "Success");

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
        model->populateMap(toRelativePathsMap(map, workDir));

        emit completeTreeModel(model);
        emit status(QString("SHA-%1: %2 files").arg(json->dbShaType).arg(map.size()));
    }
    else
        emit resetView();
}

void Manager::resetDatabase()
{
    makeJsonModel(json->filePath);
}

//making tree model | file paths : info about current availability on disk
void Manager::makeJsonModel(const QString &jsonFilePath)
{
    if(!jsonFilePath.endsWith(".ver.json")) {
        emit showMessage("Expected file extension '*.ver.json'", "Wrong DB file!");
        return;
    }

    emit status("Json Model creation...");
    parsedData = json->parseJson(jsonFilePath);

    if(parsedData.isEmpty()) {
        return;
    }

    workDir = QFileInfo(jsonFilePath).absolutePath() + '/';
    QStringList filelist = parsedData.keys();
    QStringList actualFiles = Files().actualFileListFiltered(workDir, json->filteredExtensions); // all files from workDir except filtered extensions and *.ver.json and *.sha1/256/512

    filesAvailability.clear();
    lostFiles.clear();
    newFiles.clear();

    foreach(const QString &i, filelist) {

        if (parsedData[i] == "unreadable") {
            if (QFileInfo::exists(i))
                filesAvailability[i] = "on Disk (unreadable)";
            else {
                filesAvailability[i] = "LOST file (unreadable)";
                lostFiles.append(i);
            }
        }

        else if (QFileInfo::exists(i))
            filesAvailability[i] = "on Disk";
        else {
            filesAvailability[i] = "LOST file";
            lostFiles.append(i);
        }
    }

    foreach(const QString &i, actualFiles) {
        if(!filelist.contains(i)) {
            filesAvailability[i] = "NEW file";
            newFiles.append(i);
        }
    }

    makeTreeModel(filesAvailability);

    setMode_model();

    QString tipText;

    if (newFiles.size() > 0 || lostFiles.size() > 0) {
        tipText = "\n\nUse context menu for more options";
    }

    emit showMessage(QString("Algorithm: SHA-%1\nLast update: %2\nStored size: %3\n\nStored paths: %4\nNew files: %5\nLost files: %6%7")
                     .arg(json->dbShaType).arg(json->lastUpdate, json->storedDataSize).arg(parsedData.size()).arg(newFiles.size()).arg(lostFiles.size()).arg(tipText), "Database parsed");
}

void Manager::showNewLostOnly()
{
    QMap<QString,QString> newlost;

    if (lostFiles.size() > 0) {
        foreach (const QString &file, lostFiles) {
            newlost.insert(file, "LOST file");
        }
    }

    if (newFiles.size() > 0) {
        foreach (const QString &file, newFiles) {
            newlost.insert(file, "NEW file");
        }
    }

    makeTreeModel(newlost);
}

//remove lost files, add new files
void Manager::updateNewLost()
{
    emit setMode("processing");
    QMap<QString,QString> changes; // display added/removed from database files

    if (lostFiles.size() > 0) {
        foreach (const QString &file, lostFiles) {
            parsedData.remove(file);
            changes.insert(file, "removed from DB");
        }
    }

    if (newFiles.size() > 0) {
        QMap<QString,QString> newFilesSums = shaCalc->calcShaList(newFiles, json->dbShaType);
        if(newFilesSums.isEmpty()) {
            return;
        }
        parsedData.insert(newFilesSums); // add new data to Database
        foreach (const QString &file, newFilesSums.keys()) {
            changes.insert(file, "added to DB");
        }
    }

    if (json->makeJsonDB(parsedData, json->filePath)) {
        makeTreeModel(changes);
        emit showMessage(QString("Database updated. Added %1 files, removed %2").arg(newFiles.size()).arg(lostFiles.size()));
        lostFiles.clear();
        newFiles.clear();
    }

    emit setMode("endProcess");
    emit setMode("model");
}

// update json Database with new checksums for files with failed verification
void Manager::updateMismatch()
{
    QMap<QString,QString> changes; // display files with updated checksums
    if (differences.size() > 0) {
        foreach (const QString &file, differences.keys()) {
            parsedData[file] = recalculated[file];
            changes.insert(file, "stored checksum updated");
        }
    }
    else {
        qDebug()<<"Manager::updateMismatch() | ZERO differences.size()";
        return;
    }

    if (json->makeJsonDB(parsedData, json->filePath)) {
        makeTreeModel(changes);
        setMode_model();
        emit showMessage(QString("Chechsums updated for %1 files").arg(differences.size()));
    }
}

// checking the list of files against the checksums stored in the database
void Manager::verifyFileList()
{
    if(parsedData.isEmpty()) {
        qDebug()<<"parsedData is Empty";
        return;
    }

    emit setMode("processing");

    QStringList flist;
    differences.clear();
    recalculated.clear();

    QMapIterator<QString,QString> i (filesAvailability);
    while (i.hasNext()) {
        i.next();
        if (i.value() == "on Disk") {
            flist.append(i.key());
        }
    }

    recalculated = shaCalc->calcShaList(flist, json->dbShaType);
    if(recalculated.isEmpty()) {
        return;
    }

    QMapIterator<QString,QString> ii (recalculated);

    while(ii.hasNext()) {
        ii.next();
        if (parsedData[ii.key()] != ii.value())
            differences[ii.key()] = "NOT match";
    }

    if (differences.size() > 0) {
        makeTreeModel(differences);
        emit showMessage(QString("%1 files changed or corrupted").arg(differences.size()),"FAILED");
        emit setMode("endProcess");
        emit setMode("updateMismatch");
        return;
    }
    else {
        emit showMessage(QString("ALL %1 files passed the verification.\nStored SHA-%2 chechsums matched.").arg(recalculated.size()).arg(json->dbShaType), "Success");
    }

    emit setMode("endProcess");
    //setMode_model();
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
        //emit setMode("cur");
        return;
    }

    if (!QFileInfo(checkFilePath).isFile()) {
        checkFilePath = line.mid(shaStrLen(shatype)+2).replace("\n","");
        if (!QFileInfo(checkFilePath).isFile()) {
            emit showMessage("No File to check", "Warning");
            //emit setMode("cur");
            return;
        }
    }

    emit setMode("processing");
    QString savedSum = line.mid(0, shaStrLen(shatype));
    QString sum = shaCalc->calcShaFile(checkFilePath, shatype);
    if(sum == nullptr) {
        //emit setMode("cur");
        return;
    }

    if (sum == savedSum) {
        emit showMessage("Checksum Match", "Success");
    }
    else {
        emit showMessage("Checksum does NOT match", "Failed");
    }
    emit setMode("endProcess");
    //emit setMode("cur");
}

//check only selected file instead all database cheking
void Manager::checkCurrentItemSum(const QString &path)
{
    QString filepath = workDir + path;
    qDebug()<<"Manager::checkCurrentItemSum | filepath:"<<filepath;

    if (QFileInfo(filepath).isDir()) {
        emit showMessage("Currently only files can be checked independently.\nPlease select a file or check the whole database", "Warning");
        return;
    }
    if (!QFileInfo(filepath).isFile()) {
        emit showMessage("File does not exist", "Warning");
        return;
    }

    if (!parsedData.contains(filepath)) {
        emit showMessage("Checksum is missing in the database.\nPlease Update New/Lost", "NEW File");
        return;
    }

    QString savedSum = parsedData[filepath];
    if (savedSum == "unreadable" || savedSum == "filtered") {
        emit showMessage("This file has been excluded (Filtered or Unreadable).\nNo checksum in the database.", "Excluded File");
        return;
    }

    emit setMode("processing");
    QString sum = shaCalc->calcShaFile(filepath, json->dbShaType);
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
void Manager::getFInfo(const QString &path)
{
    emit status("Counting...");

    QString text;
    QFileInfo f (path);
    if(f.isDir())
         text = Files(path).folderContentStatus();
    else if(f.isFile())
        text = QString("%1 %2").arg(f.fileName(), QLocale().formattedDataSize(f.size()));

    emit status(text);
}

//if there are New Files or Lost Files --> setMode("modelNewLost"); else setMode("model");
void Manager::setMode_model()
{
    if (newFiles.size() > 0 || lostFiles.size() > 0) {
        emit status(QString("SHA-%1: %2 new files, %3 lost files | Total files listed: %4").arg(json->dbShaType).arg(newFiles.size()).arg(lostFiles.size()).arg(filesAvailability.size()));
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
}
