#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include "files.h"
#include "jsondb.h"


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
void JsonDb::makeJson(const DataContainer &data)
{
    emit status("Exporting data to json...");
    bool isWorkDirRelative = !data.metaData.databaseFileName.contains('/');

    QJsonObject header;
    header["Created with"] = "Veretino dev_0.2.0 https://github.com/artemvlas/veretino";
    header["Files number"] = data.metaData.numChecksums;
    header["Folder"] = paths::folderName(data.metaData.workDir);
    header["Used algorithm"] = QString("SHA-%1").arg(data.metaData.shaType);
    header["Total size"] = format::dataSizeReadableExt(data.metaData.totalSize);
    header["Updated"] = format::currentDateTime();
    if (isWorkDirRelative)
        header["Working folder"] = "Relative";
    else
        header["Working folder"] = data.metaData.workDir;

    if (!data.metaData.filter.extensionsList.isEmpty()) {
        if (data.metaData.filter.include)
            header["Included Only"] = data.metaData.filter.extensionsList.join(" "); // only files with extensions from this list are included in the database, others are ignored
        else
            header["Ignored"] = data.metaData.filter.extensionsList.join(" "); // files with extensions from this list are ignored (not included in the database)
    }

    QJsonObject storedData;   
    QJsonArray unreadableFiles;

    FileList::const_iterator i;
    for (i = data.filesData.constBegin(); i != data.filesData.constEnd(); ++i) {
        if (!i.value().isReadable) {
            unreadableFiles.append(i.key());
        }
        else if (!i.value().checksum.isEmpty()) {
            storedData.insert(i.key(), i.value().checksum);
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

    QString databaseStatus = QString("Current status:\nChecksums stored: %1\nTotal size: %2")
                                 .arg(data.metaData.numChecksums).arg(format::dataSizeReadable(data.metaData.totalSize));


    QString pathToSave;
    if (isWorkDirRelative)
        pathToSave = paths::joinPath(data.metaData.workDir, data.metaData.databaseFileName);
    else
        pathToSave = data.metaData.databaseFileName;


    if (saveJsonFile(doc, pathToSave)) {
        emit status("Saved");
        emit showMessage(QString("%1\n\n%2\n\nDatabase: %3\nuse it to check the data integrity")
                             .arg(data.metaData.about, databaseStatus, QFileInfo(pathToSave).fileName()), "Success");
    }
    else {
        header["Working folder"] = data.metaData.workDir;
        mainArray[0] = header;
        doc.setArray(mainArray);

        pathToSave = paths::joinPath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
                                    QFileInfo(pathToSave).fileName());

        if (saveJsonFile(doc, pathToSave)) {
            emit status("Saved to Desktop");
            emit showMessage(QString("%1\n\n%2\n\nUnable to save in: %3\n!!! Saved to Desktop folder !!!\nDatabase: %4\nuse it to check the data integrity")
                                .arg(data.metaData.about, databaseStatus, data.metaData.workDir, QFileInfo(pathToSave).fileName()), "Warning");
        }
        else {
            emit status("NOT Saved");
            emit showMessage(QString("Unable to save json file: %1").arg(pathToSave), "Error");
        }
    }
}

DataContainer JsonDb::parseJson(const QString &filePath)
{
    QJsonArray mainArray = loadJsonDB(filePath); // json database is QJsonArray of QJsonObjects

    if (mainArray.isEmpty()) {
        return DataContainer();
    }

    emit status("Importing the Json database...");

    QJsonObject filelistData(mainArray.at(1).toObject());

    if (filelistData.isEmpty()) {
        emit showMessage(QString("%1\n\nThe database doesn't contain checksums.\nProbably all files have been ignored.")
                         .arg(QFileInfo(filePath).fileName()), "Empty Database!");
        //qDebug()<< "EMPTY filelistData";
        emit status();
        return DataContainer();
    }

    QJsonObject header(mainArray.at(0).toObject());
    DataContainer parsedData;

    // filling metadata
    if (header.contains("Working folder") && !(header.value("Working folder").toString() == "Relative")) {
        parsedData.metaData.databaseFileName = filePath;
        parsedData.metaData.workDir = header.value("Working folder").toString();
    }
    else {
        parsedData.metaData.databaseFileName = QFileInfo(filePath).fileName();
        parsedData.metaData.workDir = paths::parentFolder(filePath);
    }

    if (header.contains("Ignored")) {
        parsedData.metaData.filter.include = false;
        parsedData.metaData.filter.extensionsList = header.value("Ignored").toString().split(" ");
        //qDebug() << "jsonDB::parseJson | ignoredExtensions:" << parsedData.ignoredExtensions;
    }
    else if (header.contains("Included Only")) {
        parsedData.metaData.filter.include = true;
        parsedData.metaData.filter.extensionsList = header.value("Included Only").toString().split(" ");
        //qDebug() << "jsonDB::parseJson | Included Only:" << parsedData.onlyExtensions;
    }

    if (header.contains("Used algorithm")) {
        QString shatype = header.value("Used algorithm").toString();
        shatype.remove("SHA-");
        int type = shatype.toInt();
        if (type == 1 || type == 256 || type == 512)
            parsedData.metaData.shaType = type;
    }

    if (header.contains("Updated"))
        parsedData.metaData.saveDateTime = header.value("Updated").toString();

    if (header.contains("Total size"))
        parsedData.metaData.storedTotalSize = header.value("Total size").toString();

    if (header.contains("Files number"))
        parsedData.metaData.numChecksums = header.value("Files number").toInt();

    // populating the main data
    QJsonObject::const_iterator i;
    for (i = filelistData.constBegin(); i != filelistData.constEnd(); ++i) {
        FileValues curFileValue;
        curFileValue.checksum = i.value().toString();
        parsedData.filesData.insert(i.key(), curFileValue);
    }

    QJsonObject excludedFiles(mainArray.at(2).toObject());
    if (!excludedFiles.isEmpty()) {
        if (excludedFiles.contains("Unreadable files")) {
            QJsonArray unreadableFiles = excludedFiles.value("Unreadable files").toArray();
            for (int var = 0; var < unreadableFiles.size(); ++var) {
                FileValues curFileValues;
                curFileValues.isReadable = false;
                parsedData.filesData.insert(unreadableFiles.at(var).toString(), curFileValues);
            }
        }
    }

    return parsedData;
}
