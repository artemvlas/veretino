#include "jsondb.h"

jsonDB::jsonDB(const QString &path, QObject *parent)
    : QObject{parent}
{
    if (path != nullptr) {
        if (QFileInfo(path).isFile())
            filePath = path;
        else if (QFileInfo(path).isDir())
            folderPath = path;
        else
            qDebug()<<"JsonDB: Wrong input path"<<path;
    }
}

QJsonDocument jsonDB::readJsonFile(const QString &pathToFile)
{
    if (pathToFile != nullptr)
        filePath = pathToFile;

    QFile jsonFile(filePath);
    if(jsonFile.open(QFile::ReadOnly))
        return QJsonDocument().fromJson(jsonFile.readAll());
    else {
        emit showMessage("Error while reading Json File", "Error");
        return QJsonDocument();
    }
}

bool jsonDB::saveJsonFile(const QJsonDocument &document, const QString &pathToFile)
{
    if (pathToFile != nullptr)
        filePath = pathToFile;

    QFile jsonFile(filePath);
    if(!jsonFile.open(QFile::WriteOnly))
        return false;
    if(jsonFile.write(document.toJson()))
        return true;
    else
        return false;
}

QJsonArray jsonDB::loadJsonDB(const QString &pathToFile)
{
    if (pathToFile != nullptr)
        filePath = pathToFile;

    if(readJsonFile(filePath).isArray()) {
        QJsonArray dataArray = readJsonFile(filePath).array();
        if(dataArray.size() >= 2 || dataArray[0].isObject() || dataArray[1].isObject())
            return readJsonFile(filePath).array();
        else {
            emit showMessage("Corrupted Json/Database", "Error");
            return QJsonArray();
        }
    }
    else {
        emit showMessage("Corrupted Json/Database", "Error");
        return QJsonArray();
    }
}

// making "checksums... .ver.json" database from QMAP {full file path : checksum or info(unreadable, filtered)}
bool jsonDB::makeJsonDB(const QMap<QString,QString> &dataMap, const QString &filepath)
{
    if (dataMap.isEmpty())
        return false;

    if (filepath != nullptr)
        filePath = filepath;

    qint64 totalSize = 0;
    QDir dir (QFileInfo(filePath).absolutePath());

    QJsonDocument doc;
    QJsonArray data;
    QJsonObject header;
    QJsonObject computedData;
    QJsonObject excludedFiles;
    QJsonArray unreadableFiles;
    QString relativePath;

    QMapIterator<QString,QString> i(dataMap);
    while(i.hasNext()) {
        i.next();
        relativePath = dir.relativeFilePath(i.key());

        if(i.value() == "unreadable") {
            unreadableFiles.append(relativePath);
        }
        else {
            computedData.insert(relativePath, i.value());
            totalSize += QFileInfo(i.key()).size();
        }
    }

    if (unreadableFiles.size() > 0)
        excludedFiles["Unreadable files"] = unreadableFiles;

    int shatype = shatypeByLen(computedData.begin().value().toString().size());

    QLocale locale (QLocale::English);
    header["Created with"] = "Veretino 0.1.0 https://github.com/artemvlas/veretino";
    header["Files number"] = computedData.size();
    header["Folder"] = dir.dirName();
    header["SHA type"] = QString("SHA-%1").arg(shatype);
    header["Total size"] = QString("%1 (%2 bytes)").arg(locale.formattedDataSize(totalSize), locale.toString(totalSize));
    header["Updated"] = QDateTime::currentDateTime().toString("yyyy/MM/dd HH:mm");
    header["Working folder"] = "Relative"; // functionality to work with this variable will be realized in the next versions

    if (!filteredExtensions.isEmpty()) {
        header["Filtered"] = filteredExtensions.join(" ");
        filteredExtensions.clear();
    }

    data.append(header);
    data.append(computedData);
    data.append(excludedFiles);

    doc.setArray(data);

    if(saveJsonFile(doc,filePath))
        return true;
    else {
        emit showMessage("Unable to save json file","Error");
        return false;
    }
}

//returns {"file path" : "checksum" or "unreadable"}
QMap<QString,QString> jsonDB::parseJson(const QString &pathToFile)
{
    if(pathToFile != nullptr)
        filePath = pathToFile;

    QString workDir = QFileInfo(filePath).absolutePath() + '/';
    filePath = pathToFile;

    QJsonArray mainArray = loadJsonDB(filePath);

    if (mainArray.isEmpty()) {
        return QMap<QString,QString>();
    }

    QJsonObject header (mainArray[0].toObject());
    QJsonObject jsonData (mainArray[1].toObject());
    QJsonObject excludedFiles (mainArray[2].toObject());
    QMap<QString,QString> parsedData;

    if (header.contains("Filtered")) {
        filteredExtensions = header["Filtered"].toString().split(" ");
        qDebug()<< "jsonDB::parseJson | filteredExtensions:" << filteredExtensions;
    }
    else {
        filteredExtensions.clear();
    }

    foreach (const QString &file, jsonData.keys()) {
        parsedData[workDir + file] = jsonData[file].toString(); // from relative to full path
    }

    dbShaType = shatypeByLen(parsedData.begin().value().size());
    // if something wrong with first value, try others
    if (dbShaType == 0) {
        foreach (const QString &v, parsedData.values()) {
            int t = shatypeByLen(v.size());
            if(t != 0)
                dbShaType = t;
        }
    }

    if (!excludedFiles.isEmpty()) {
        if(excludedFiles.contains("Unreadable files")) {
            QJsonArray unreadableFiles = excludedFiles["Unreadable files"].toArray();
            for (int var = 0; var < unreadableFiles.size(); ++var) {
                parsedData[workDir + unreadableFiles.at(var).toString()] = "unreadable";
            }
        }
    }

    return parsedData;
}

int jsonDB::shatypeByLen(const int &len)
{
    if (len == 40)
        return 1;
    else if (len == 64)
        return 256;
    else if (len == 128)
        return 512;
    else
        return 0;
}
