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

const QString JsonDb::h_key_DateTime = QStringLiteral(u"DateTime");
const QString JsonDb::h_key_Ignored = QStringLiteral(u"Ignored");
const QString JsonDb::h_key_Included = QStringLiteral(u"Included");
const QString JsonDb::h_key_Algo = QStringLiteral(u"Hash Algorithm");
const QString JsonDb::h_key_WorkDir = QStringLiteral(u"WorkDir");
const QString JsonDb::h_key_Flags = QStringLiteral(u"Flags");

const QString JsonDb::h_key_Updated = QStringLiteral(u"Updated");
const QString JsonDb::h_key_Verified = QStringLiteral(u"Verified");

const QString JsonDb::a_key_Unreadable = QStringLiteral(u"Unreadable files");

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

    const QJsonDocument readedDoc = readJsonFile(filePath);

    if (readedDoc.isArray()) {
        const QJsonArray dataArray = readedDoc.array();
        if (dataArray.size() > 1 && dataArray.at(0).isObject() && dataArray.at(1).isObject())
            return dataArray;
    }

    emit showMessage("Corrupted or unreadable Json Database:\n" + filePath, "Error");
    return QJsonArray();
}

QJsonObject JsonDb::dbHeader(const DataContainer *data, const QModelIndex &rootFolder)
{
    const bool isBranching = TreeModel::isFolderRow(rootFolder);
    const Numbers &_num = isBranching ? data->getNumbers(rootFolder) : data->numbers_;
    const MetaData &_meta = data->metaData_;
    QJsonObject header;

    static const QString _app_origin = tools::joinStrings(Lit::s_appNameVersion, Lit::s_webpage, QStringLiteral(u" >> "));
    header[QStringLiteral(u"App/Origin")] = _app_origin;
    header[QStringLiteral(u"Folder")] = isBranching ? rootFolder.data().toString() : paths::basicName(_meta.workDir);
    header[QStringLiteral(u"Total Checksums")] = _num.numberOf(FileStatus::CombHasChecksum);
    header[QStringLiteral(u"Total Size")] = format::dataSizeReadableExt(_num.totalSize(FileStatus::CombAvailable));
    header[h_key_Algo] = format::algoToStr(_meta.algorithm);

    // DateTime
    if (isBranching && data->isAllMatched(_num)) {
        header[h_key_DateTime] = QStringLiteral(u"Created: ") + format::currentDateTime();
    } else {
        header[h_key_DateTime] = QString("%1, %2, %3").arg(_meta.datetime[DTstr::DateCreated],
                                                           _meta.datetime[DTstr::DateUpdated],
                                                           _meta.datetime[DTstr::DateVerified]);
    }

    // WorkDir
    if (!isBranching && !data->isWorkDirRelative())
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
        else if (iter.status() & FileStatus::CombUnreadable) {
            unreadableFiles.append(iter.path(rootFolder));
        }
    }

    QJsonObject excludedFiles;
    if (unreadableFiles.size() > 0)
        excludedFiles[a_key_Unreadable] = unreadableFiles;

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
                                                     : data->metaData_.dbFilePath; // main database

    if (saveJsonFile(doc, pathToSave)) {
        emit setStatusbarText(QStringLiteral(u"Saved"));
        return pathToSave;
    }
    else {
        header[h_key_WorkDir] = rootFolder.isValid() ? paths::parentFolder(pathToSave) : data->metaData_.workDir;
        mainArray[0] = header;
        doc.setArray(mainArray);

        QString resPathToSave = paths::joinPath(Files::desktopFolderPath, paths::basicName(pathToSave));

        if (saveJsonFile(doc, resPathToSave)) {
            emit setStatusbarText("Saved to Desktop");
            return resPathToSave;
        }
        else {
            emit setStatusbarText("NOT Saved");
            emit showMessage("Unable to save json file: " + pathToSave, "Error");
            return QString();
        }
    }
}

FileValues JsonDb::makeFileValues(const QString &filePath, const QString &basicDate)
{
    if (QFileInfo::exists(filePath)) {
        QFileInfo _fi(filePath);
        FileStatus _status = (!basicDate.isEmpty()
                              && tools::isLater(basicDate, _fi.lastModified()))
                                 ? FileStatus::NotCheckedMod : FileStatus::NotChecked;

        return FileValues(_status, _fi.size());
    }

    return FileValues(FileStatus::Missing);
}

DataContainer* JsonDb::parseJson(const QString &filePath)
{
    // the database is QJsonArray of QJsonObjects [{}, {}, ...]
    const QJsonArray mainArray = loadJsonDB(filePath);

    if (mainArray.isEmpty()) {
        return nullptr;
    }

    emit setStatusbarText(QStringLiteral(u"Importing Json database..."));

    const QJsonObject filelistData = mainArray.at(1).toObject();

    if (filelistData.isEmpty()) {
        emit showMessage(paths::basicName(filePath) + "\n\n"
                            "The database doesn't contain checksums.\n"
                            "Probably all files have been ignored.", "Empty Database!");
        emit setStatusbarText();
        return nullptr;
    }

    const QJsonObject _header = mainArray.at(0).toObject();
    DataContainer *parsedData = new DataContainer(getMetaData(filePath, _header, filelistData));
    const QString &workDir = parsedData->metaData_.workDir;

    // populating the main data
    emit setStatusbarText(QStringLiteral(u"Parsing Json database..."));
    const QString _basicDT = considerFileModDate ? parsedData->basicDate() : QString();

    for (QJsonObject::const_iterator it = filelistData.constBegin();
         !isCanceled() && it != filelistData.constEnd(); ++it)
    {
        const QString _fullPath = paths::joinPath(workDir, it.key());
        FileValues _values = makeFileValues(_fullPath, _basicDT);
        _values.checksum = it.value().toString();

        const QModelIndex &_item = parsedData->model_->add_file(it.key(), _values);

        // add missing items to cache
        if (_cacheMissingChecksums && (_values.status == FileStatus::Missing))
            parsedData->_cacheMissing.insert(_values.checksum, _item);
    }

    // additional data
    QSet<QString> _unrCache;
    const QJsonObject excludedFiles = (mainArray.size() > 2) ? mainArray.at(2).toObject() : QJsonObject();
    if (excludedFiles.contains(a_key_Unreadable)) {
        const QJsonArray unreadableFiles = excludedFiles.value(a_key_Unreadable).toArray();

        for (int var = 0; !isCanceled() && var < unreadableFiles.size(); ++var) {
            const QString _relPath = unreadableFiles.at(var).toString();
            const QString _fullPath = paths::joinPath(workDir, _relPath);
            const FileStatus _status = tools::failedCalcStatus(_fullPath);

            if (_status & FileStatus::CombUnreadable) {
                parsedData->model_->add_file(_relPath, FileValues(_status));
                _unrCache << _relPath;
            }
        }
    }

    // adding new files
    emit setStatusbarText(QStringLiteral(u"Looking for new files..."));
    QDirIterator it(workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        const QString _fullPath = it.next();
        const QString _relPath = paths::relativePath(workDir, _fullPath);

        if (parsedData->metaData_.filter.isFileAllowed(_relPath)
            && !filelistData.contains(_relPath)
            && !_unrCache.contains(_relPath)
            && it.fileInfo().isReadable())
        {
            parsedData->model_->add_file(_relPath, FileValues(FileStatus::New, it.fileInfo().size()));
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

    return parsedData;
}

MetaData JsonDb::getMetaData(const QString &filePath, const QJsonObject &header, const QJsonObject &fileList)
{
    MetaData _meta;
    _meta.dbFilePath = filePath;
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
        _meta.algorithm = tools::algoByStrLen(fileList.begin().value().toString().length());
        //qDebug() << "JsonDb::getMetaData | The algorithm is determined by the length of the checksum string:" << metaData.algorithm;
    }

    // [date]
    // compatibility with previous versions
    if (header.contains(h_key_Updated)) {
        _meta.datetime[DTstr::DateUpdated] = QString("%1: %2").arg(h_key_Updated, header.value(h_key_Updated).toString());
        if (header.contains(h_key_Verified))
            _meta.datetime[DTstr::DateVerified] = QString("%1: %2").arg(h_key_Verified, header.value(h_key_Verified).toString());
    }
    else { // [datetime] version 0.4.0+
        const QString _strDateTime = findValueStr(header, QStringLiteral(u"time"));
        const QStringList _dtList = _strDateTime.split(Lit::s_sepCommaSpace);

        if (_dtList.size() == 3) {
            for (int i = 0; i < _dtList.size(); ++i) { // && i < 3
                _meta.datetime[i] = _dtList.at(i);
            }
        } else {
            for (const QString &_dt : _dtList) {
                if (_dt.isEmpty())
                    continue;

                if (_dt.startsWith('C'))
                    _meta.datetime[DTstr::DateCreated] = _dt;
                else if (_dt.startsWith('U'))
                    _meta.datetime[DTstr::DateUpdated] = _dt;
                else if (_dt.startsWith('V'))
                    _meta.datetime[DTstr::DateVerified] = _dt;
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
