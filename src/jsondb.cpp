// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "jsondb.h"
#include "files.h"
#include "tools.h"
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include "treemodeliterator.h"
#include <QDirIterator>

JsonDb::JsonDb(QObject *parent)
    : QObject(parent)
{}

JsonDb::JsonDb(const QString &filePath, QObject *parent)
    : QObject(parent), jsonFilePath(filePath)
{}

QJsonDocument JsonDb::readJsonFile(const QString &filePath)
{
    QFile jsonFile(filePath);
    if (jsonFile.open(QFile::ReadOnly))
        return QJsonDocument().fromJson(jsonFile.readAll());
    else {
        emit showMessage("Error while reading the Json File", "Error");
        return QJsonDocument();
    }
}

bool JsonDb::saveJsonFile(const QJsonDocument &document, const QString &filePath)
{
    QFile jsonFile(filePath);
    return (jsonFile.open(QFile::WriteOnly) && jsonFile.write(document.toJson()));
}

QJsonArray JsonDb::loadJsonDB(const QString &filePath)
{
    if (readJsonFile(filePath).isArray()) {
        QJsonArray dataArray = readJsonFile(filePath).array();
        if (dataArray.size() > 1 && dataArray.at(0).isObject() && dataArray.at(1).isObject())
            return readJsonFile(filePath).array();
    }

    emit showMessage("Corrupted Json/Database", "Error");
    return QJsonArray();
}

// making "checksums... .ver.json" database
JsonDb::Result JsonDb::makeJson(const DataContainer* data)
{
    if (!data || data->model_->isEmpty()) {
        qDebug() << "JsonDb::makeJson | no data to make json db";
        return NotSaved;
    }

    canceled = false;
    emit setStatusbarText("Exporting data to json...");

    bool isWorkDirRelative = (data->metaData.workDir == paths::parentFolder(data->metaData.databaseFilePath));

    QJsonObject header;
    header["Created with"] = QString("Veretino %1 https://github.com/artemvlas/veretino").arg(APP_VERSION);
    header["Files number"] = data->numbers.numChecksums;
    header["Folder"] = paths::basicName(data->metaData.workDir);
    header[strHeaderAlgo] = format::algoToStr(data->metaData.algorithm);
    header["Total size"] = format::dataSizeReadableExt(data->numbers.totalSize);
    header["Updated"] = data->metaData.saveDateTime;
    header[strHeaderWorkDir] = isWorkDirRelative ? "Current" : data->metaData.workDir;

    if (data->isFilterApplied()) {
        if (data->metaData.filter.isFilter(FilterRule::Include))
            header[strHeaderIncluded] = data->metaData.filter.extensionsList.join(" "); // only files with extensions from this list are included in the database, others are ignored
        else if (data->metaData.filter.isFilter(FilterRule::Ignore))
            header[strHeaderIgnored] = data->metaData.filter.extensionsList.join(" "); // files with extensions from this list are ignored (not included in the database)
    }

    QJsonObject storedData;   
    QJsonArray unreadableFiles;

    TreeModelIterator iter(data->model_);

    while (iter.hasNext() && !canceled) {
        iter.nextFile();
        if (iter.data(Column::ColumnStatus).value<FileStatus>() == FileStatus::Unreadable) {
            unreadableFiles.append(iter.path());
        }
        else {
            QString checksum = iter.data(Column::ColumnChecksum).toString();
            if (!checksum.isEmpty())
                storedData.insert(iter.path(), checksum);
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

    QJsonDocument doc;
    doc.setArray(mainArray);

    QString databaseStatus = QString("Checksums stored: %1\nTotal size: %2")
                                    .arg(data->numbers.numChecksums)
                                    .arg(format::dataSizeReadable(data->numbers.totalSize));

    if (canceled) {
        qDebug() << "JsonDb::makeJson | Canceled";
        return Canceled;
    }

    QString pathToSave;

    if (saveJsonFile(doc, data->metaData.databaseFilePath)) {
        emit setStatusbarText("Saved");
        emit showMessage(QString("%1\n\n%2\n\nDatabase: %3\nuse it to check the data integrity")
                             .arg(data->metaData.about, databaseStatus, data->databaseFileName()), "Success");
        return Saved;
    }
    else {
        header[strHeaderWorkDir] = data->metaData.workDir;
        mainArray[0] = header;
        doc.setArray(mainArray);

        pathToSave = paths::joinPath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                                                             data->databaseFileName());

        if (saveJsonFile(doc, pathToSave)) {
            emit setStatusbarText("Saved to Desktop");
            emit showMessage(QString("%1\n\n%2\n\nUnable to save in: %3\n!!! Saved to Desktop folder !!!\nDatabase: %4\nuse it to check the data integrity")
                                 .arg(data->metaData.about, databaseStatus, data->metaData.workDir, data->databaseFileName()), "Warning");
            return SavedToDesktop;
        }

        else {
            emit setStatusbarText("NOT Saved");
            emit showMessage(QString("Unable to save json file: %1").arg(data->metaData.databaseFilePath), "Error");
            return NotSaved;
        }
    }
}

DataContainer* JsonDb::parseJson(const QString &filePath)
{
    elapsedTimer.start();

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
    QJsonObject header(mainArray.at(0).toObject());
    DataContainer* parsedData = new DataContainer;

    // filling metadata
    parsedData->metaData.databaseFilePath = filePath;

    QString strIgnored = tools::findCompleteString(header.keys(), strHeaderIgnored);
    QString strIncluded = tools::findCompleteString(header.keys(), strHeaderIncluded);
    QString strWorkDir = tools::findCompleteString(header.keys(), strHeaderWorkDir);

    parsedData->metaData.workDir = (!strWorkDir.isEmpty() && header.value(strWorkDir).toString().contains('/'))
                                       ? header.value(strWorkDir).toString()
                                       : paths::parentFolder(filePath);

    if (!strIgnored.isEmpty()) {
        parsedData->metaData.filter.setFilter(FilterRule::Ignore, header.value(strIgnored).toString().split(" "));
    }
    else if (!strIncluded.isEmpty()) {
        parsedData->metaData.filter.setFilter(FilterRule::Include, header.value(strIncluded).toString().split(" "));
    }

    if (header.contains(strHeaderAlgo)) {
        parsedData->metaData.algorithm = tools::strToAlgo(header.value(strHeaderAlgo).toString());
        qDebug() << "JsonDb::parseJson | Used algorithm from header data:" << parsedData->metaData.algorithm;
    }
    else {
        parsedData->metaData.algorithm = tools::algorithmByStrLen(filelistData.begin().value().toString().length());
        qDebug() << "JsonDb::parseJson | The algorithm is determined by the length of the checksum string:" << parsedData->metaData.algorithm;
    }

    if (header.contains("Updated"))
        parsedData->metaData.saveDateTime = header.value("Updated").toString();

    // with very big Data, it is faster to add new files first, and then the main ones
    emit setStatusbarText("Looking for new files...");

    QDir dir(parsedData->metaData.workDir);
    QDirIterator it(parsedData->metaData.workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !canceled) {
        QString fullPath = it.next();
        QString relPath = dir.relativeFilePath(fullPath);

        if (paths::isFileAllowed(relPath, parsedData->metaData.filter) && !filelistData.contains(relPath)) {
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

    qDebug() << "JsonDb::parseJson | parsing took" << format::millisecToReadable(elapsedTimer.elapsed(), false);

    return parsedData;
}

void JsonDb::cancelProcess()
{
    canceled = true;
}
