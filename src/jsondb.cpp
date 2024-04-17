/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "jsondb.h"
#include "files.h"
#include "tools.h"
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include "treemodeliterator.h"
#include "datamaintainer.h"
#include <QDirIterator>

JsonDb::JsonDb(QObject *parent)
    : QObject(parent)
{}

JsonDb::JsonDb(const QString &filePath, QObject *parent)
    : QObject(parent), jsonFilePath(filePath)
{}

QJsonDocument JsonDb::readJsonFile(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        qDebug() << "JsonDb::readJsonFile | File not found:" << filePath;
        return QJsonDocument();
    }

    QFile jsonFile(filePath);

    return jsonFile.open(QFile::ReadOnly) ? QJsonDocument::fromJson(jsonFile.readAll()) : QJsonDocument();
}

bool JsonDb::saveJsonFile(const QJsonDocument &document, const QString &filePath)
{
    QFile jsonFile(filePath);
    return (jsonFile.open(QFile::WriteOnly) && jsonFile.write(document.toJson()));
}

QJsonArray JsonDb::loadJsonDB(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
        emit showMessage("File not found:\n" + filePath, "Error");
        return QJsonArray();
    }

    QJsonDocument readedDoc = readJsonFile(filePath);

    if (readedDoc.isArray()) {
        QJsonArray dataArray = readedDoc.array();
        if (dataArray.size() > 1 && dataArray.at(0).isObject() && dataArray.at(1).isObject())
            return dataArray;
    }

    emit showMessage("Corrupted or unreadable Json Database", "Error");
    return QJsonArray();
}

bool JsonDb::updateSuccessfulCheckDateTime(const QString &filePath)
{
    QJsonArray dataArray = loadJsonDB(filePath);
    if (dataArray.isEmpty())
        return false;

    QJsonObject header = dataArray.at(0).toObject();
    header.insert("Verified", format::currentDateTime());
    dataArray[0] = header;

    return saveJsonFile(QJsonDocument(dataArray), filePath);
}

QJsonObject JsonDb::dbHeader(const DataContainer *data, const QModelIndex &rootFolder)
{
    QJsonObject header;
    bool isWorkDirRelative = (data->metaData.workDir == paths::parentFolder(data->metaData.databaseFilePath));
    Numbers numbers = DataContainer::getNumbers(data->model_, rootFolder);

    header["Created with"] = QString("Veretino %1 https://github.com/artemvlas/veretino").arg(APP_VERSION);
    header["Files number"] = numbers.numberOf(FileStatus::FlagHasChecksum);
    header["Folder"] = rootFolder.isValid() ? rootFolder.data().toString() : paths::basicName(data->metaData.workDir);
    header[strHeaderAlgo] = format::algoToStr(data->metaData.algorithm);
    header["Total size"] = format::dataSizeReadableExt(numbers.totalSize(FileStatus::FlagAvailable));
    header["Updated"] = data->metaData.saveDateTime;

    if (!isWorkDirRelative && !rootFolder.isValid())
        header.insert(strHeaderWorkDir, data->metaData.workDir);

    if (!data->metaData.successfulCheckDateTime.isEmpty() && !rootFolder.isValid())
        header.insert("Verified", data->metaData.successfulCheckDateTime);

    if (data->isFilterApplied()) {
        if (data->metaData.filter.isFilter(FilterRule::Include))
            header[strHeaderIncluded] = data->metaData.filter.extensionsList.join(" "); // only files with extensions from this list are included in the database, others are ignored
        else if (data->metaData.filter.isFilter(FilterRule::Ignore))
            header[strHeaderIgnored] = data->metaData.filter.extensionsList.join(" "); // files with extensions from this list are ignored (not included in the database)
    }

    return header;
}

// returns the path to the file if the write was successful, otherwise an empty string
QString JsonDb::makeJson(const DataContainer* data, const QModelIndex &rootFolder)
{
    if (!data || data->model_->isEmpty()) {
        qDebug() << "JsonDb::makeJson | no data to make json db";
        return QString();
    }

    QJsonObject header = dbHeader(data, rootFolder);

    canceled = false;
    emit setStatusbarText("Exporting data to json...");

    QJsonObject storedData;   
    QJsonArray unreadableFiles;

    TreeModelIterator iter(data->model_, rootFolder);

    while (iter.hasNext() && !canceled) {
        iter.nextFile();
        if (iter.status() == FileStatus::Unreadable) {
            unreadableFiles.append(iter.path(rootFolder));
        }
        else {
            QString checksum = iter.data(Column::ColumnChecksum).toString();
            if (!checksum.isEmpty())
                storedData.insert(iter.path(rootFolder), checksum);
        }
    }

    QJsonObject excludedFiles;
    if (unreadableFiles.size() > 0)
        excludedFiles["Unreadable files"] = unreadableFiles;

    QJsonArray mainArray;
    mainArray.append(header);
    mainArray.append(storedData);
    if (!excludedFiles.isEmpty())
        mainArray.append(excludedFiles);

    QJsonDocument doc(mainArray);

    if (canceled) {
        qDebug() << "JsonDb::makeJson | Canceled";
        return QString();
    }

    QString pathToSave;

    pathToSave = rootFolder.isValid() ? data->branchDbFilePath(rootFolder) // branching
                                      : data->metaData.databaseFilePath; // main database

    if (saveJsonFile(doc, pathToSave)) {
        emit setStatusbarText("Saved");
        return pathToSave;
    }
    else {
        header[strHeaderWorkDir] = rootFolder.isValid() ? paths::parentFolder(pathToSave) : data->metaData.workDir;
        mainArray[0] = header;
        doc.setArray(mainArray);

        QString resPathToSave = paths::joinPath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                paths::basicName(pathToSave));

        if (saveJsonFile(doc, resPathToSave)) {
            emit setStatusbarText("Saved to Desktop");
            return resPathToSave;
        }

        else {
            emit setStatusbarText("NOT Saved");
            emit showMessage(QString("Unable to save json file: %1").arg(pathToSave), "Error");
            return QString();
        }
    }
}

DataContainer* JsonDb::parseJson(const QString &filePath)
{
    //elapsedTimer.start();

    QJsonArray mainArray = loadJsonDB(filePath); // json database is QJsonArray of QJsonObjects

    if (mainArray.isEmpty()) {
        return nullptr;
    }

    emit setStatusbarText("Importing Json database...");

    QJsonObject filelistData(mainArray.at(1).toObject());

    if (filelistData.isEmpty()) {
        emit showMessage(QString("%1\n\nThe database doesn't contain checksums.\nProbably all files have been ignored.")
                                                                    .arg(paths::basicName(filePath)), "Empty Database!");
        emit setStatusbarText();
        return nullptr;
    }

    canceled = false;
    DataContainer* parsedData = new DataContainer(getMetaData(filePath, mainArray.at(0).toObject(), filelistData));

    // Filling in the Main Data
    // with very big Data, it is faster to add new files first, and then the main ones
    emit setStatusbarText("Looking for new files...");

    QDir dir(parsedData->metaData.workDir);
    QDirIterator it(parsedData->metaData.workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !canceled) {
        QString fullPath = it.next();
        QString relPath = dir.relativeFilePath(fullPath);

        if (parsedData->metaData.filter.isFileAllowed(relPath) && !filelistData.contains(relPath)) {
            QFileInfo fileInfo(fullPath);
            if (fileInfo.isReadable()) {
                FileValues curFileValues(FileStatus::New);
                curFileValues.size = fileInfo.size();
                parsedData->model_->addFile(relPath, curFileValues);
            }
        }
    }

    // populating the main data
    emit setStatusbarText("Parsing Json database...");
    QJsonObject::const_iterator i;
    for (i = filelistData.constBegin(); !canceled && i != filelistData.constEnd(); ++i) {
        FileValues curFileValues;

        QString fullPath = paths::joinPath(parsedData->metaData.workDir, i.key());
        if (QFileInfo::exists(fullPath)) {
            curFileValues.size = QFileInfo(fullPath).size();
            curFileValues.status = FileStatus::NotChecked;
        }
        else
            curFileValues.status = FileStatus::Missing;

        curFileValues.checksum = i.value().toString();

        parsedData->model_->addFile(i.key(), curFileValues);
    }

    QJsonObject excludedFiles(mainArray.at(2).toObject());
    if (!excludedFiles.isEmpty()) {
        if (excludedFiles.contains("Unreadable files")) {
            QJsonArray unreadableFiles = excludedFiles.value("Unreadable files").toArray();
            for (int var = 0; !canceled && var < unreadableFiles.size(); ++var) {                
                parsedData->model_->addFile(unreadableFiles.at(var).toString(), FileValues(FileStatus::Unreadable));
            }
        }
    }

    emit setStatusbarText();

    if (canceled) {
        qDebug() << "JsonDb::parseJson | Canceled:" << paths::basicName(filePath);
        delete parsedData;
        return nullptr;
    }

    //qDebug() << "JsonDb::parseJson | parsing took" << format::millisecToReadable(elapsedTimer.elapsed(), false);

    return parsedData;
}

MetaData JsonDb::getMetaData(const QString &filePath, const QJsonObject &header, const QJsonObject &fileList)
{
    MetaData metaData;
    metaData.databaseFilePath = filePath;
    metaData.isImported = true;

    // [checking for files in the intended WorkDir]
    QString strWorkDir = findValueStr(header, strHeaderWorkDir);

    if (strWorkDir.contains('/')) {
        if (!isPresentInWorkDir(strWorkDir, fileList)
            && isPresentInWorkDir(paths::parentFolder(filePath), fileList))
            metaData.workDir = paths::parentFolder(filePath);
        else
            metaData.workDir = strWorkDir;
    }
    else
        metaData.workDir = paths::parentFolder(filePath);

    // [filter rule]
    QString strIgnored = findValueStr(header, strHeaderIgnored);
    QString strIncluded = findValueStr(header, strHeaderIncluded);

    if (!strIgnored.isEmpty()) {
        metaData.filter.setFilter(FilterRule::Ignore, strIgnored.split(" "));
    }
    else if (!strIncluded.isEmpty()) {
        metaData.filter.setFilter(FilterRule::Include, strIncluded.split(" "));
    }

    // [algorithm]
    QString strAlgo = findValueStr(header, strHeaderAlgo);
    if (!strAlgo.isEmpty()) {
        metaData.algorithm = tools::strToAlgo(strAlgo);
        qDebug() << "JsonDb::getMetaData | Used algorithm from header data:" << metaData.algorithm;
    }
    else {
        metaData.algorithm = tools::algorithmByStrLen(fileList.begin().value().toString().length());
        qDebug() << "JsonDb::getMetaData | The algorithm is determined by the length of the checksum string:" << metaData.algorithm;
    }

    // [date]
    if (header.contains("Updated"))
        metaData.saveDateTime = header.value("Updated").toString();

    if (header.contains("Verified"))
        metaData.successfulCheckDateTime = header.value("Verified").toString();

    return metaData;
}

// checks if there is at least one file from the list (keys) in the folder (workDir)
bool JsonDb::isPresentInWorkDir(const QString &workDir, const QJsonObject &fileList)
{
    if (!QFileInfo::exists(workDir))
        return false;

    bool isPresent = false;
    QJsonObject::const_iterator i;

    for (i = fileList.constBegin(); !canceled && i != fileList.constEnd(); ++i) {
        QString fullPath = paths::joinPath(workDir, i.key());
        if (QFileInfo::exists(fullPath)) {
            isPresent = true;
            break;
        }
    }

    return isPresent;
}

QString JsonDb::findValueStr(const QJsonObject &object, const QString &approxKey, int sampleLength)
{
    QString result;
    QJsonObject::const_iterator i;

    for (i = object.constBegin(); i != object.constEnd(); ++i) {
        if (i.key().contains(approxKey.left(sampleLength), Qt::CaseInsensitive)) {
            result = i.value().toString();
            break;
        }
    }

    return result;
}

void JsonDb::cancelProcess()
{
    canceled = true;
}
