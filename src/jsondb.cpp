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

const QString JsonDb::h_key_DateTime = "DateTime";
const QString JsonDb::h_key_Ignored = "Ignored";
const QString JsonDb::h_key_Included = "Included";
const QString JsonDb::h_key_Algo = "Hash Algorithm";
const QString JsonDb::h_key_WorkDir = "WorkDir";
const QString JsonDb::h_key_Flags = "Flags";

const QString JsonDb::h_key_Updated = "Updated";
const QString JsonDb::h_key_Verified = "Verified";

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
    return (proc_ && proc_->isCanceled());
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
    const MetaData &_meta = data->metaData;
    const Numbers &_numbers = DataContainer::getNumbers(data->model_, rootFolder);
    QJsonObject header;

    header[QStringLiteral(u"App/Origin")] = APP_NAME_VERSION + QStringLiteral(u" >> https://github.com/artemvlas/veretino");
    header[QStringLiteral(u"Folder")] = rootFolder.isValid() ? rootFolder.data().toString() : paths::basicName(_meta.workDir);
    header[QStringLiteral(u"Total Checksums")] = _numbers.numberOf(FileStatus::CombHasChecksum);
    header[QStringLiteral(u"Total Size")] = format::dataSizeReadableExt(_numbers.totalSize(FileStatus::CombAvailable));
    header[h_key_Algo] = format::algoToStr(_meta.algorithm);

    // DateTime
    header[h_key_DateTime] = QString("%1, %2, %3").arg(_meta.datetime[DateTimeStr::DateCreated],
                                                       _meta.datetime[DateTimeStr::DateUpdated],
                                                       _meta.datetime[DateTimeStr::DateVerified]);

    // WorkDir
    if (!data->isWorkDirRelative() && !rootFolder.isValid())
        header[h_key_WorkDir] = _meta.workDir;

    // Filter
    if (data->isFilterApplied()) {
        const bool _inc = _meta.filter.isFilter(FilterRule::Include);
        header[_inc ? h_key_Included : h_key_Ignored] = _meta.filter.extensionString();
    }

    // Flags (needs to be redone after expanding the flags list)
    if (_meta.flags)
        header[h_key_Flags] = QStringLiteral(u"const");

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

    emit setStatusbarText(QStringLiteral(u"Exporting data to json..."));

    QJsonObject storedData;
    QJsonArray unreadableFiles;

    TreeModelIterator iter(data->model_, rootFolder);

    while (iter.hasNext() && !isCanceled()) {
        iter.nextFile();
        const QString checksum = iter.checksum();
        if (!checksum.isEmpty()) {
            storedData.insert(iter.path(rootFolder), checksum);
        }
        else if (iter.status() == FileStatus::Unreadable) {
            unreadableFiles.append(iter.path(rootFolder));
        }
    }

    QJsonObject excludedFiles;
    if (unreadableFiles.size() > 0)
        excludedFiles[QStringLiteral(u"Unreadable files")] = unreadableFiles;

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
        emit setStatusbarText(QStringLiteral(u"Saved"));
        return pathToSave;
    }
    else {
        header[h_key_WorkDir] = rootFolder.isValid() ? paths::parentFolder(pathToSave) : data->metaData.workDir;
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

    emit setStatusbarText(QStringLiteral(u"Importing Json database..."));

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
    emit setStatusbarText(QStringLiteral(u"Looking for new files..."));

    QDirIterator it(workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        const QString _fullPath = it.next();
        const QString _relPath = paths::relativePath(workDir, _fullPath);

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
    emit setStatusbarText(QStringLiteral(u"Parsing Json database..."));
    QJsonObject::const_iterator i;
    for (i = filelistData.constBegin(); !isCanceled() && i != filelistData.constEnd(); ++i) {
        const QString _fullPath = paths::joinPath(workDir, i.key());
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
        if (excludedFiles.contains(QStringLiteral(u"Unreadable files"))) {
            const QJsonArray &unreadableFiles = excludedFiles.value(QStringLiteral(u"Unreadable files")).toArray();

            for (int var = 0; !isCanceled() && var < unreadableFiles.size(); ++var) {
                const QString _unrFile = unreadableFiles.at(var).toString();
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
    MetaData _meta;
    _meta.databaseFilePath = filePath;
    _meta.dbFileState = MetaData::Saved;

    // [checking for files in the intended WorkDir]
    const QString strWorkDir = findValueStr(header, h_key_WorkDir);
    const bool isSpecWorkDir = strWorkDir.contains('/') && (isPresentInWorkDir(strWorkDir, fileList)
                                                            || !isPresentInWorkDir(paths::parentFolder(filePath), fileList));

    _meta.workDir = isSpecWorkDir ? strWorkDir : paths::parentFolder(filePath);

    // [filter rule]
    const QString strIgnored = findValueStr(header, h_key_Ignored);
    const QString strIncluded = findValueStr(header, h_key_Included);

    if (!strIgnored.isEmpty()) {
        _meta.filter.setFilter(FilterRule::Ignore, strIgnored);
    }
    else if (!strIncluded.isEmpty()) {
        _meta.filter.setFilter(FilterRule::Include, strIncluded);
    }

    // [algorithm]
    const QString strAlgo = findValueStr(header, QStringLiteral(u"Algo"));
    if (!strAlgo.isEmpty()) {
        _meta.algorithm = tools::strToAlgo(strAlgo);
        //qDebug() << "JsonDb::getMetaData | Used algorithm from header data:" << metaData.algorithm;
    }
    else {
        _meta.algorithm = tools::algorithmByStrLen(fileList.begin().value().toString().length());
        //qDebug() << "JsonDb::getMetaData | The algorithm is determined by the length of the checksum string:" << metaData.algorithm;
    }

    // [date]
    // compatibility with previous versions
    if (header.contains(h_key_Updated)) {
        _meta.datetime[DateTimeStr::DateUpdated] = QString("%1: %2").arg(h_key_Updated, header.value(h_key_Updated).toString());
        if (header.contains(h_key_Verified))
            _meta.datetime[DateTimeStr::DateVerified] = QString("%1: %2").arg(h_key_Verified, header.value(h_key_Verified).toString());
    }
    else { // [datetime] version 0.4.0+
        const QString _strDateTime = findValueStr(header, QStringLiteral(u"time"));
        const QStringList &_dtList = _strDateTime.split(", ");

        if (_dtList.size() == 3) {
            for (int i = 0; i < _dtList.size(); ++i) { // && i < 3
                _meta.datetime[i] = _dtList.at(i);
            }
        } else {
            for (const QString &_dt : _dtList) {
                if (_dt.startsWith('C'))
                    _meta.datetime[DateTimeStr::DateCreated] = _dt;
                else if (_dt.startsWith('U'))
                    _meta.datetime[DateTimeStr::DateUpdated] = _dt;
                else if (_dt.startsWith('V'))
                    _meta.datetime[DateTimeStr::DateVerified] = _dt;
            }
        }
    }

    // [flags]
    const QString _strFlags = header.value(h_key_Flags).toString();
    if (_strFlags.contains(QStringLiteral(u"const")))
        _meta.flags |= MetaData::FlagConst;

    return _meta;
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
