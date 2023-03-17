#include "jsondb.h"

jsonDB::jsonDB(const QString &path, QObject *parent)
    : QObject{parent}
{
    if (path != nullptr) {
        if (QFileInfo(path).isFile() && path.endsWith(".ver.json", Qt::CaseInsensitive))
            filePath = path;
        else if (QFileInfo(path).isDir())
            folderPath = path;
        else
            qDebug()<<"JsonDB: Wrong input path" << path;
    }
}

QJsonDocument jsonDB::readJsonFile(const QString &pathToFile)
{
    if (pathToFile != nullptr)
        filePath = pathToFile;

    QFile jsonFile(filePath);
    if (jsonFile.open(QFile::ReadOnly))
        return QJsonDocument().fromJson(jsonFile.readAll());
    else {
        emit showMessage("Error while reading the Json File", "Error");
        return QJsonDocument();
    }
}

bool jsonDB::saveJsonFile(const QJsonDocument &document, const QString &pathToFile)
{
    if (pathToFile != nullptr)
        filePath = pathToFile;

    QFile jsonFile(filePath);
    if (!jsonFile.open(QFile::WriteOnly))
        return false;

    return (jsonFile.write(document.toJson()));
}

QJsonArray jsonDB::loadJsonDB(const QString &pathToFile)
{
    if (pathToFile != nullptr)
        filePath = pathToFile;

    if (readJsonFile(filePath).isArray()) {
        QJsonArray dataArray = readJsonFile(filePath).array();
        if(dataArray.size() >= 2 && dataArray[0].isObject() && dataArray[1].isObject())
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

// making "checksums... .ver.json" database from DataContainer
bool jsonDB::makeJsonDB(DataContainer *data)
{
    if (data == nullptr)
        return false;

    if (data->jsonFilePath != nullptr)
        filePath = data->jsonFilePath;

    qint64 totalSize = 0;
    QDir dir (QFileInfo(filePath).absolutePath());

    QJsonDocument doc;
    QJsonArray mainArray;
    QJsonObject header;
    QJsonObject computedData;
    QJsonObject excludedFiles;
    QJsonArray unreadableFiles;
    QString relativePath;

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

    int shatype = shatypeByLen(computedData.begin().value().toString().size());

    QLocale locale (QLocale::English);
    header["Created with"] = "Veretino 0.1.2 https://github.com/artemvlas/veretino";
    header["Files number"] = computedData.size();
    header["Folder"] = dir.dirName();
    header["SHA type"] = QString("SHA-%1").arg(shatype);
    header["Total size"] = QString("%1 (%2 bytes)").arg(locale.formattedDataSize(totalSize), locale.toString(totalSize));
    header["Updated"] = QDateTime::currentDateTime().toString("yyyy/MM/dd HH:mm");
    header["Working folder"] = "Relative"; // functionality to work with this variable will be realized in the next versions

    if (!data->ignoredExtensions.isEmpty()) {
        header["Ignored"] = data->ignoredExtensions.join(" ");
    }
    else if (!data->onlyExtensions.isEmpty()) {
        header["Included Only"] = data->onlyExtensions.join(" ");
    }

    mainArray.append(header);
    mainArray.append(computedData);
    if (!excludedFiles.isEmpty())
        mainArray.append(excludedFiles);

    doc.setArray(mainArray);

    if (saveJsonFile(doc, filePath))
        return true;
    else {
        emit showMessage(QString("Unable to save json file: %1").arg(filePath), "Error");
        return false;
    }
}

DataContainer* jsonDB::parseJson(const QString &pathToFile)
{
    if (pathToFile != nullptr)
        filePath = pathToFile;

    QJsonArray mainArray = loadJsonDB(filePath); // json database is QJsonArray of QJsonObjects

    if (mainArray.isEmpty()) {
        return nullptr;
    }

    QJsonObject header (mainArray[0].toObject());
    QJsonObject filelistData (mainArray[1].toObject());
    QJsonObject excludedFiles (mainArray[2].toObject());

    if (filelistData.isEmpty()) {
        qDebug()<<"EMPTY filelistData";
        return nullptr;
    }

    DataContainer *data = new DataContainer(filePath);

    if (header.contains("Ignored")) {
        data->setIgnoredExtensions(header["Ignored"].toString().split(" "));
        qDebug()<< "jsonDB::parseJson | ignoredExtensions:" << data->ignoredExtensions;
    }
    else if (header.contains("Included Only")) {
        data->setOnlyExtensions(header["Included Only"].toString().split(" "));
        qDebug()<< "jsonDB::parseJson | Included Only:" << data->onlyExtensions;
    }

    if (header.contains("SHA type")) {
        QString shatype = header["SHA type"].toString();
        shatype.remove("SHA-");
        data->dbShaType = shatype.toInt();
    }

    // if the shatype has not readed from the json file, try to determine it by the length of the checksum string
    if (data->dbShaType == 0)
        data->dbShaType = shatypeByLen(data->mainData.begin().value().size());

    QJsonObject::const_iterator i;
    QString curFolder = data->workDir + '/';
    for (i = filelistData.constBegin(); i != filelistData.constEnd(); ++i) {
        data->mainData.insert(curFolder + i.key(), i.value().toString()); // from relative to full path
    }    

    if (!excludedFiles.isEmpty()) {
        if (excludedFiles.contains("Unreadable files")) {
            QJsonArray unreadableFiles = excludedFiles["Unreadable files"].toArray();
            for (int var = 0; var < unreadableFiles.size(); ++var) {
                data->mainData[curFolder + unreadableFiles.at(var).toString()] = "unreadable";
            }
        }
    }

    if (header.contains("Updated"))
        data->lastUpdate = header["Updated"].toString();
    else
        data->lastUpdate = QString();

    if (header.contains("Total size"))
        data->storedDataSize = header["Total size"].toString();
    else
        data->storedDataSize = QString();

    return data;
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
