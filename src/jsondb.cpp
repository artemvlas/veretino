#include "jsondb.h"
#include "QFile"
#include "QDir"
#include "files.h"
#include "QStandardPaths"

jsonDB::jsonDB(QObject *parent)
    : QObject{parent}
{}

QJsonDocument jsonDB::readJsonFile(const QString &filePath)
{
    QFile jsonFile(filePath);
    if (jsonFile.open(QFile::ReadOnly))
        return QJsonDocument().fromJson(jsonFile.readAll());
    else {
        emit showMessage("Error while reading the Json File", "Error");
        return QJsonDocument();
    }
}

bool jsonDB::saveJsonFile(const QJsonDocument &document, const QString &filePath)
{
    QFile jsonFile(filePath);
    return (jsonFile.open(QFile::WriteOnly) && jsonFile.write(document.toJson()));
}

QJsonArray jsonDB::loadJsonDB(const QString &filePath)
{
    if (readJsonFile(filePath).isArray()) {
        QJsonArray dataArray = readJsonFile(filePath).array();
        if (dataArray.size() >= 2 && dataArray[0].isObject() && dataArray[1].isObject())
            return readJsonFile(filePath).array();
    }

    emit showMessage("Corrupted Json/Database", "Error");
    return QJsonArray();

}

// making "checksums... .ver.json" database from DataContainer
void jsonDB::makeJson(DataContainer *data, const QString &about)
{
    if (data == nullptr)
        return;

    QString filePath = data->jsonFilePath;

    QDir dir (data->workDir);

    QJsonDocument doc;
    QJsonArray mainArray;
    QJsonObject header;
    QJsonObject computedData;
    QJsonObject excludedFiles;
    QJsonArray unreadableFiles;
    QString relativePath;

    qint64 totalSize = 0;
    QMapIterator<QString,QString> i(data->mainData);
    while (i.hasNext()) {
        i.next();
        relativePath = dir.relativeFilePath(i.key());

        if (i.value() == "unreadable") {
            unreadableFiles.append(relativePath);
        }
        else {
            computedData.insert(relativePath, i.value());
            totalSize += QFileInfo(i.key()).size();
        }
    }

    if (unreadableFiles.size() > 0)
        excludedFiles["Unreadable files"] = unreadableFiles;

    int shatype = data->shaType();

    QLocale locale (QLocale::English);
    header["Created with"] = "Veretino 0.1.3 https://github.com/artemvlas/veretino";
    header["Files number"] = computedData.size();
    header["Folder"] = dir.dirName();
    header["Used algorithm"] = QString("SHA-%1").arg(shatype);
    header["Total size"] = QString("%1 (%2 bytes)").arg(locale.formattedDataSize(totalSize), locale.toString(totalSize));
    header["Updated"] = QDateTime::currentDateTime().toString("yyyy/MM/dd HH:mm");

    if (data->workDir == Files::parentFolder(filePath))
        header["Working folder"] = "Relative";
    else
        header["Working folder"] = data->workDir;

    if (!data->ignoredExtensions.isEmpty()) {
        header["Ignored"] = data->ignoredExtensions.join(" ");
    }
    else if (!data->onlyExtensions.isEmpty()) {
        header["Included Only"] = data->onlyExtensions.join(" ");
    }

    QString databaseStatus = QString("Current status:\nChecksums stored: %1\nTotal size: %2").arg(computedData.size()).arg(header.value("Total size").toString());

    mainArray.append(header);
    mainArray.append(computedData);
    if (!excludedFiles.isEmpty())
        mainArray.append(excludedFiles);

    doc.setArray(mainArray);

    if (saveJsonFile(doc, filePath))
        emit showMessage(QString("%1\n\n%2\n\nDatabase: %3\nuse it to check the data integrity").arg(about, databaseStatus, QFileInfo(filePath).fileName()), "Success");
    else {
        QString workFolder = Files::parentFolder(filePath);
        header["Working folder"] = workFolder;
        mainArray[0] = header;
        doc.setArray(mainArray);

        if (saveJsonFile(doc, QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + '/' + QFileInfo(filePath).fileName())) {
            emit showMessage(QString("%1\n\n%2\n\nUnable to save in: %3\n!!! Saved to Desktop folder !!!\nDatabase: %4\nuse it to check the data integrity")
                                .arg(about, databaseStatus, workFolder, QFileInfo(filePath).fileName()), "Warning");
        }
        else {
            emit showMessage(QString("Unable to save json file: %1").arg(filePath), "Error");
        }
    }
}

DataContainer* jsonDB::parseJson(const QString &filePath)
{
    QJsonArray mainArray = loadJsonDB(filePath); // json database is QJsonArray of QJsonObjects

    if (mainArray.isEmpty()) {
        return nullptr;
    }

    QJsonObject header (mainArray[0].toObject());
    QJsonObject filelistData (mainArray[1].toObject());
    QJsonObject excludedFiles (mainArray[2].toObject());

    if (filelistData.isEmpty()) {
        emit showMessage(QString("%1\n\nThe database doesn't contain checksums.\nProbably all files have been ignored.")
                         .arg(QFileInfo(filePath).fileName()), "Empty Database!");
        //qDebug()<< "EMPTY filelistData";
        return nullptr;
    }

    DataContainer *data = new DataContainer(filePath);

    if (header.contains("Working folder") && !(header.value("Working folder").toString() == "Relative"))
        data->workDir = header.value("Working folder").toString();

    if (header.contains("Ignored")) {
        data->setIgnoredExtensions(header.value("Ignored").toString().split(" "));
        qDebug()<< "jsonDB::parseJson | ignoredExtensions:" << data->ignoredExtensions;
    }
    else if (header.contains("Included Only")) {
        data->setOnlyExtensions(header.value("Included Only").toString().split(" "));
        qDebug()<< "jsonDB::parseJson | Included Only:" << data->onlyExtensions;
    }

    if (header.contains("Used algorithm")) {
        QString shatype = header.value("Used algorithm").toString();
        shatype.remove("SHA-");
        int type = shatype.toInt();
        if (type == 1 || type == 256 || type == 512)
            data->shatype = type;
    }

    QJsonObject::const_iterator i;
    QString curFolder = data->workDir + '/';
    for (i = filelistData.constBegin(); i != filelistData.constEnd(); ++i) {
        data->mainData.insert(curFolder + i.key(), i.value().toString()); // from relative to full path
    }    

    if (!excludedFiles.isEmpty()) {
        if (excludedFiles.contains("Unreadable files")) {
            QJsonArray unreadableFiles = excludedFiles.value("Unreadable files").toArray();
            for (int var = 0; var < unreadableFiles.size(); ++var) {
                data->mainData[curFolder + unreadableFiles.at(var).toString()] = "unreadable";
            }
        }
    }

    if (header.contains("Updated"))
        data->lastUpdate = header.value("Updated").toString();
    else
        data->lastUpdate = QString();

    if (header.contains("Total size"))
        data->storedDataSize = header.value("Total size").toString();
    else
        data->storedDataSize = QString();

    return data;
}
