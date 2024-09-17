/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "jsondb.h"
#include "files.h"
#include "tools.h"
#include <QFile>
#include <QFileInfo>
#include "treemodeliterator.h"
#include <QDirIterator>
#include <QElapsedTimer>

JsonDb::JsonDb(QObject *parent)
    : QObject(parent)
{}

JsonDb::JsonDb(const QString &filePath, QObject *parent)
    : QObject(parent), jsonFilePath(filePath)
{}

void JsonDb::setProcState(const ProcState *procState)
{
    proc_ = procState;
}

bool JsonDb::isCanceled() const
{
    return proc_ && proc_->isCanceled();
}

QJsonDocument JsonDb::readJsonFile(const QString &filePath)
{
    if (!QFile::exists(filePath)) {
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

    const QJsonDocument &readedDoc = readJsonFile(filePath);

    if (readedDoc.isArray()) {
        const QJsonArray &dataArray = readedDoc.array();
        if (dataArray.size() > 1 && dataArray.at(0).isObject() && dataArray.at(1).isObject())
            return dataArray;
    }

    emit showMessage("Corrupted or unreadable Json Database:\n" + filePath, "Error");
    return QJsonArray();
}

QJsonObject JsonDb::dbHeader(const DataContainer *data, const QModelIndex &rootFolder)
{
    const MetaData &meta = data->metaData;
    const Numbers &numbers = DataContainer::getNumbers(data->model_, rootFolder);
    QJsonObject header;

    header[QStringLiteral(u"App/Origin")] = QString("%1 >> https://github.com/artemvlas/veretino").arg(APP_NAME_VERSION);
    header[QStringLiteral(u"Folder")] = rootFolder.isValid() ? rootFolder.data().toString() : paths::basicName(meta.workDir);
    header[strHeaderAlgo] = format::algoToStr(meta.algorithm);
    header[QStringLiteral(u"Total Checksums")] = numbers.numberOf(FileStatus::CombHasChecksum);
    header[QStringLiteral(u"Total Size")] = format::dataSizeReadableExt(numbers.totalSize(FileStatus::CombAvailable));

    // DateTime
    header[strHeaderDateTime] = QString("%1, %2, %3").arg(meta.datetime[DateTimeStr::DateCreated],
                                                          meta.datetime[DateTimeStr::DateUpdated],
                                                          meta.datetime[DateTimeStr::DateVerified]);

    // WorkDir
    if (!data->isWorkDirRelative() && !rootFolder.isValid())
        header[strHeaderWorkDir] = meta.workDir;

    // Filter
    if (data->isFilterApplied()) {
        QString _strFilterKey = meta.filter.isFilter(FilterRule::Include) ? strHeaderIncluded : strHeaderIgnored;
        header[_strFilterKey] = meta.filter.extensionString();
    }

    // Flags (needs to be redone after expanding the flags list)
    if (meta.flags)
        header["Flags"] = "const";

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

    emit setStatusbarText("Exporting data to json...");

    QJsonObject storedData;
    QJsonArray unreadableFiles;

    TreeModelIterator iter(data->model_, rootFolder);

    while (iter.hasNext() && !isCanceled()) {
        iter.nextFile();
        const QString &checksum = iter.checksum();
        if (!checksum.isEmpty()) {
            storedData.insert(iter.path(rootFolder), checksum);
        }
        else if (iter.status() == FileStatus::Unreadable) {
            unreadableFiles.append(iter.path(rootFolder));
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

    if (isCanceled()) {
        qDebug() << "JsonDb::makeJson | Canceled";
        return QString();
    }

    const QString &pathToSave = rootFolder.isValid() ? data->getBranchFilePath(rootFolder) // branching
                                                     : data->metaData.databaseFilePath; // main database

    if (saveJsonFile(doc, pathToSave)) {
        emit setStatusbarText("Saved");
        return pathToSave;
    }
    else {
        header[strHeaderWorkDir] = rootFolder.isValid() ? paths::parentFolder(pathToSave) : data->metaData.workDir;
        mainArray[0] = header;
        doc.setArray(mainArray);

        QString resPathToSave = paths::joinPath(Files::desktopFolderPath, paths::basicName(pathToSave));

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
    QElapsedTimer timer;
    timer.start();

    // the database is QJsonArray of QJsonObjects [{}, {}, ...]
    const QJsonArray &mainArray = loadJsonDB(filePath);

    if (mainArray.isEmpty()) {
        return nullptr;
    }

    emit setStatusbarText("Importing Json database...");

    const QJsonObject &filelistData = mainArray.at(1).toObject();

    if (filelistData.isEmpty()) {
        emit showMessage(QString("%1\n\n"
                                 "The database doesn't contain checksums.\n"
                                 "Probably all files have been ignored.")
                                .arg(paths::basicName(filePath)), "Empty Database!");
        emit setStatusbarText();
        return nullptr;
    }

    DataContainer *parsedData = new DataContainer(getMetaData(filePath, mainArray.at(0).toObject(), filelistData));
    const QString &workDir = parsedData->metaData.workDir;

    // adding new files
    emit setStatusbarText("Looking for new files...");

    QDirIterator it(workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        const QString &_fullPath = it.next();
        const QString &_relPath = paths::relativePath(workDir, _fullPath);

        if (parsedData->metaData.filter.isFileAllowed(_relPath)
            && !filelistData.contains(_relPath))
        {
            QFileInfo fileInfo(_fullPath);
            if (fileInfo.isReadable()) {
                parsedData->model_->add_file(_relPath, FileValues(FileStatus::New, fileInfo.size()));
            }
        }
    }

    // populating the main data
    emit setStatusbarText("Parsing Json database...");
    QJsonObject::const_iterator i;
    for (i = filelistData.constBegin(); !isCanceled() && i != filelistData.constEnd(); ++i) {
        const QString &_fullPath = paths::joinPath(workDir, i.key());
        const bool _exist = QFileInfo::exists(_fullPath);
        qint64 _size = _exist ? QFileInfo(_fullPath).size() : -1;
        FileStatus _status = _exist ? FileStatus::NotChecked : FileStatus::Missing;

        FileValues _values(_status, _size);
        _values.checksum = i.value().toString();

        parsedData->model_->add_file(i.key(), _values);
    }

    // additional data
    const QJsonObject &excludedFiles = (mainArray.size() > 2) ? mainArray.at(2).toObject() : QJsonObject();
    if (!excludedFiles.isEmpty()) {
        if (excludedFiles.contains("Unreadable files")) {
            const QJsonArray &unreadableFiles = excludedFiles.value("Unreadable files").toArray();

            for (int var = 0; !isCanceled() && var < unreadableFiles.size(); ++var) {
                const QString &_unrFile = unreadableFiles.at(var).toString();
                if (QFileInfo::exists(paths::joinPath(workDir, _unrFile))) {
                    parsedData->model_->add_file(_unrFile, FileValues(FileStatus::Unreadable));
                }
            }
        }
    }

    // end
    emit setStatusbarText();

    if (isCanceled()) {
        qDebug() << "JsonDb::parseJson | Canceled:" << paths::basicName(filePath);
        delete parsedData;
        return nullptr;
    }

    parsedData->model_->clearCacheFolderItems();
    qDebug() << "Parsing time:" << timer.elapsed();

    return parsedData;
}

MetaData JsonDb::getMetaData(const QString &filePath, const QJsonObject &header, const QJsonObject &fileList)
{
    MetaData metaData;
    metaData.databaseFilePath = filePath;
    metaData.dbFileState = MetaData::Saved;

    // [checking for files in the intended WorkDir]
    const QString strWorkDir = findValueStr(header, strHeaderWorkDir);
    const bool isSpecWorkDir = strWorkDir.contains('/') && (isPresentInWorkDir(strWorkDir, fileList)
                                                            || !isPresentInWorkDir(paths::parentFolder(filePath), fileList));

    metaData.workDir = isSpecWorkDir ? strWorkDir : paths::parentFolder(filePath);

    // [filter rule]
    const QString strIgnored = findValueStr(header, strHeaderIgnored);
    const QString strIncluded = findValueStr(header, strHeaderIncluded);

    if (!strIgnored.isEmpty()) {
        metaData.filter.setFilter(FilterRule::Ignore, strIgnored);
    }
    else if (!strIncluded.isEmpty()) {
        metaData.filter.setFilter(FilterRule::Include, strIncluded);
    }

    // [algorithm]
    const QString strAlgo = findValueStr(header, QStringLiteral(u"Algo"));
    if (!strAlgo.isEmpty()) {
        metaData.algorithm = tools::strToAlgo(strAlgo);
        //qDebug() << "JsonDb::getMetaData | Used algorithm from header data:" << metaData.algorithm;
    }
    else {
        metaData.algorithm = tools::algorithmByStrLen(fileList.begin().value().toString().length());
        //qDebug() << "JsonDb::getMetaData | The algorithm is determined by the length of the checksum string:" << metaData.algorithm;
    }

    // [date]
    // compatibility with previous versions
    static const QString _strUpdated = "Updated";
    if (header.contains(_strUpdated)) {
        metaData.datetime[DateTimeStr::DateUpdated] = QString("%1 %2").arg(_strUpdated, header.value(_strUpdated).toString());

        static const QString _strVerified = "Verified";
        if (header.contains(_strVerified))
            metaData.datetime[DateTimeStr::DateVerified] = QString("%1 %2").arg(_strVerified, header.value(_strVerified).toString());
    }
    else { // [datetime] version 0.4.0+
        const QString _strDateTime = findValueStr(header, QStringLiteral(u"time"));
        const QStringList &_dtList = _strDateTime.split(", ");

        for (int i = 0; i < _dtList.size() && i < 3; ++i) {
            metaData.datetime[i] = _dtList.at(i);
        }
    }

    // [Flags]
    const QString _strFlags = header.value(QStringLiteral(u"Flags")).toString();
    if (_strFlags.contains(QStringLiteral(u"const")))
        metaData.flags |= MetaData::FlagConst;

    return metaData;
}

// checks if there is at least one file from the list (keys) in the folder (workDir)
bool JsonDb::isPresentInWorkDir(const QString &workDir, const QJsonObject &fileList)
{
    if (!QFileInfo::exists(workDir))
        return false;

    QJsonObject::const_iterator i;

    for (i = fileList.constBegin(); !isCanceled() && i != fileList.constEnd(); ++i) {
        if (QFileInfo::exists(paths::joinPath(workDir, i.key()))) {
            return true;
        }
    }

    return false;
}

QString JsonDb::findValueStr(const QJsonObject &object, const QString &approxKey, int sampleLength)
{
    QJsonObject::const_iterator i;

    for (i = object.constBegin(); i != object.constEnd(); ++i) {
        if (i.key().contains(approxKey.left(sampleLength), Qt::CaseInsensitive)) {
            return i.value().toString();
        }
    }

    return QString();
}
