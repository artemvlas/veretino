/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "datamaintainer.h"
#include <QFileInfo>
#include <QDebug>
#include <QDirIterator>
#include "treemodeliterator.h"
#include "tools.h"
#include "pathstr.h"

DataMaintainer::DataMaintainer(QObject *parent)
    : QObject(parent)
{
    connections();
}

DataMaintainer::DataMaintainer(DataContainer* initData, QObject *parent)
    : QObject(parent)
{
    connections();
    setSourceData(initData);
}

void DataMaintainer::connections()
{
    connect(this, &DataMaintainer::numbersUpdated, this, [=]{ if (m_data->contains(FileStatus::Mismatched | FileStatus::Missing))
                                                                  m_data->m_metadata.datetime.m_verified.clear(); });
}

void DataMaintainer::setProcState(const ProcState *procState)
{
    proc_ = procState;
}

void DataMaintainer::setSourceData()
{
    setSourceData(new DataContainer(this));
}

void DataMaintainer::setSourceData(const MetaData &meta)
{
    setSourceData(new DataContainer(meta, this));
}

bool DataMaintainer::setSourceData(DataContainer *sourceData)
{
    if (sourceData) {
        clearOldData();
        m_oldData = m_data;
        m_data = sourceData;
        m_data->setParent(this);
        updateNumbers();
    }

    return sourceData; // false if sourceData == nullptr, else true
}

void DataMaintainer::clearData()
{
    if (m_data) {
        delete m_data;
        m_data = nullptr;
    }
}

void DataMaintainer::clearOldData()
{
    if (m_oldData) {
        delete m_oldData;
        m_oldData = nullptr;
    }
}

bool DataMaintainer::setItemValue(const QModelIndex &fileIndex, Column column, const QVariant &value)
{
    return (m_data && m_data->m_model->setData(fileIndex.siblingAtColumn(column), value));
}

void DataMaintainer::setFileStatus(const QModelIndex &index, FileStatus status)
{
    setItemValue(index, Column::ColumnStatus, status);
}

void DataMaintainer::setConsiderDateModified(bool consider)
{
    considerFileModDate = consider;
}

void DataMaintainer::updateDateTime()
{
    if (m_data) {
        if (m_data->isDbFileState(DbFileState::NoFile)) {
            m_data->m_metadata.datetime.update(VerDateTime::Created);
        }
        else if (m_data->contains(FileStatus::CombDbChanged)) {
            m_data->m_metadata.datetime.update(VerDateTime::Updated);
        }
    }
}

void DataMaintainer::updateVerifDateTime()
{
    if (m_data && m_data->isAllMatched()) {
        m_data->m_metadata.datetime.update(VerDateTime::Verified);
        setDbFileState(DbFileState::NotSaved);
    }
}

// adds the WorkDir contents to the m_data->m_model
int DataMaintainer::folderBasedData(FileStatus fileStatus)
{
    if (!m_data)
        return 0;

    const QString &_workDir = m_data->m_metadata.workDir;
    const FilterRule &_filter = m_data->m_metadata.filter;

    if (!QFileInfo(_workDir).isDir()) {
        qDebug() << "DM::folderBasedData | Wrong path:" << _workDir;
        return 0;
    }

    emit setStatusbarText(QStringLiteral(u"Creating a list of files..."));

    int numAdded = 0;

    QDirIterator it(_workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        const QString _fullPath = it.next();
        const QString _relPath = pathstr::relativePath(_workDir, _fullPath);

        if (_filter.isFileAllowed(_relPath)) {
            const bool _isReadable = it.fileInfo().isReadable();

            if (!_isReadable && _filter.hasAttribute(FilterAttribute::IgnoreUnpermitted)
                || _filter.hasAttribute(FilterAttribute::IgnoreSymlinks) && it.fileInfo().isSymLink())
            {
                continue;
            }

            const FileValues &_values = _isReadable ? FileValues(fileStatus, it.fileInfo().size())
                                                    : FileValues(FileStatus::UnPermitted);

            m_data->m_model->add_file(_relPath, _values);
            ++numAdded;
        }
    }

    if (isCanceled() || numAdded == 0) {
        qDebug() << "DM::folderBasedData | Canceled/No items:" << _workDir;
        emit setStatusbarText();
        emit failedDataCreation();
        clearData();
        return 0;
    }

    //if (numAdded > 0)
    updateNumbers();

    m_data->m_model->clearCacheFolderItems();
    return numAdded;
}

void DataMaintainer::updateNumbers()
{
    if (!m_data) {
        qDebug() << "DM::updateNumbers | NO m_data";
        return;
    }

    m_data->updateNumbers();
    emit numbersUpdated();
}

// if only one file has changed, there is no need to iterate over the entire list
void DataMaintainer::updateNumbers(const QModelIndex &fileIndex, const FileStatus statusBefore)
{
    updateNumbers(statusBefore,
                  TreeModel::itemFileStatus(fileIndex),
                  TreeModel::itemFileSize(fileIndex));
}

void DataMaintainer::updateNumbers(const FileStatus status_old, const FileStatus status_new, const qint64 size)
{
    if (m_data
        && m_data->m_numbers.moveFile(status_old, status_new, size))
    {
        emit numbersUpdated();
    }
}

void DataMaintainer::moveNumbers(const FileStatus before, const FileStatus after)
{
    if (m_data && m_data->m_numbers.changeStatus(before, after))
        emit numbersUpdated();
}

void DataMaintainer::setDbFileState(DbFileState state)
{
    if (m_data && !m_data->isDbFileState(state)) {
        m_data->m_metadata.dbFileState = state;
        emit dbFileStateChanged(state);
    }
}

bool DataMaintainer::updateChecksum(const QModelIndex &fileRowIndex, const QString &computedChecksum)
{
    if (!m_data || !fileRowIndex.isValid() || fileRowIndex.model() != m_data->m_model) {
        qDebug() << "DM::updateChecksum >> Error";
        return false;
    }

    const QString storedChecksum = TreeModel::itemFileChecksum(fileRowIndex);

    if (storedChecksum.isEmpty()) {
        if (!tryMoved(fileRowIndex, computedChecksum)) {
            setItemValue(fileRowIndex, Column::ColumnChecksum, computedChecksum);
            setFileStatus(fileRowIndex, FileStatus::Added);
        }
        return true;
    }
    else if (storedChecksum == computedChecksum) {
        setFileStatus(fileRowIndex, FileStatus::Matched);
        return true;
    }
    else {
        setItemValue(fileRowIndex, Column::ColumnReChecksum, computedChecksum);
        setFileStatus(fileRowIndex, FileStatus::Mismatched);
        return false;
    }
}

bool DataMaintainer::importChecksum(const QModelIndex &file, const QString &checksum)
{
    if (TreeModel::hasStatus(FileStatus::New, file) && !checksum.isEmpty()) {
        setItemValue(file, Column::ColumnChecksum, checksum);
        setFileStatus(file, FileStatus::Imported);
        return true;

        // m_data->numbers_.moveFile(FileStatus::New, FileStatus::Imported, TreeModel::itemFileSize(_file));
    }

    return false;
}

int DataMaintainer::changeFilesStatus(const FileStatuses flags, const FileStatus newStatus, const QModelIndex &rootIndex)
{
    if (!m_data || tools::isFlagCombined(newStatus)) {
        qDebug() << "DM::changeFilesStatus >> Error";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(m_data->m_model, rootIndex);

    while (iter.hasNext()) {
        if (flags & iter.nextFile().status()) {
            setFileStatus(iter.index(), newStatus);
            ++number;
        }
    }

    if (number > 0)
        updateNumbers();

    return number;
}

int DataMaintainer::addToQueue(const FileStatuses flags, const QModelIndex &rootIndex)
{
    return changeFilesStatus(flags, FileStatus::Queued, rootIndex);
}

void DataMaintainer::clearChecksum(const QModelIndex &fileIndex)
{
    setItemValue(fileIndex, Column::ColumnChecksum);
}

int DataMaintainer::clearChecksums(const FileStatuses flags, const QModelIndex &rootIndex)
{
    if (!m_data) {
        qDebug() << "DM::clearChecksums | NO m_data";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(m_data->m_model, rootIndex);

    while (iter.hasNext()) {
        if (flags & iter.nextFile().status()) {
            setItemValue(iter.index(), Column::ColumnChecksum);
            ++number;
        }
    }

    return number;
}

int DataMaintainer::clearLostFiles()
{
    if (!m_data) {
        qDebug() << "DM::clearLostFiles | NO m_data";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(m_data->m_model);

    while (iter.hasNext()) {
        if (itemFileRemoveLost(iter.nextFile().index()))
            ++number;
    }

    if (number > 0) {
        setDbFileState(DbFileState::NotSaved);
        //updateNumbers();
        moveNumbers(FileStatus::Missing, FileStatus::Removed); // experimental
    }

    return number;
}

bool DataMaintainer::itemFileRemoveLost(const QModelIndex &fileIndex)
{
    if (m_data && TreeModel::hasStatus(FileStatus::Missing, fileIndex)) {
        clearChecksum(fileIndex);
        setFileStatus(fileIndex, FileStatus::Removed);
        return true;
    }

    return false;
}

bool DataMaintainer::tryMoved(const QModelIndex &file, const QString &checksum)
{
    if (!m_data || !m_data->_cacheMissing.contains(checksum))
        return false;

    // moved out
    QModelIndex _i_movedout = m_data->_cacheMissing.take(checksum);
    const FileStatus _status = TreeModel::itemFileStatus(_i_movedout);

    if (_status & (FileStatus::Missing | FileStatus::Removed)) {
        clearChecksum(_i_movedout);
        setFileStatus(_i_movedout, FileStatus::MovedOut);
        m_data->m_numbers.moveFile(_status, FileStatus::MovedOut);

        // moved
        setFileStatus(file, FileStatus::Moved);
        setItemValue(file, Column::ColumnChecksum, checksum);
        return true;
    }

    return false;
}

int DataMaintainer::updateMismatchedChecksums()
{
    if (!m_data) {
        qDebug() << "DM::updateMismatchedChecksums | NO m_data";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(m_data->m_model);

    while (iter.hasNext()) {
        if (itemFileUpdateChecksum(iter.nextFile().index()))
            ++number;
    }

    if (number > 0) {
        setDbFileState(DbFileState::NotSaved);
        updateNumbers();
    }

    return number;
}

bool DataMaintainer::itemFileUpdateChecksum(const QModelIndex &fileIndex)
{
    if (m_data && TreeModel::hasReChecksum(fileIndex)) {
        const QString _reChecksum = TreeModel::itemFileReChecksum(fileIndex);

        if (!_reChecksum.isEmpty()) {
            setItemValue(fileIndex, Column::ColumnChecksum, _reChecksum);
            setItemValue(fileIndex, Column::ColumnReChecksum);
            setItemValue(fileIndex, Column::ColumnStatus, FileStatus::Updated);
            return true;
        }
    }

    return false;
}

void DataMaintainer::rollBackStoppedCalc(const QModelIndex &rootIndex, FileStatus prevStatus)
{
    if (prevStatus != FileStatus::Queued) { // m_data && !m_data->isInCreation()
        if (prevStatus == FileStatus::New)
            clearChecksums(FileStatus::Added, rootIndex);
        else if (tools::isFlagCombined(prevStatus)) // CombNotChecked == NotChecked | NotCheckedMod
            prevStatus = FileStatus::NotChecked;

        changeFilesStatus((FileStatus::CombProcessing | FileStatus::Added), prevStatus, rootIndex);
    }
}

// checks if there is at least one file from the list (keys) in the folder (workDir)
bool DataMaintainer::isPresentInWorkDir(const QString &workDir, const QJsonObject &fileList) const
{
    if (!QFileInfo::exists(workDir))
        return false;

    QJsonObject::const_iterator i;

    for (i = fileList.constBegin(); !isCanceled() && i != fileList.constEnd(); ++i) {
        if (QFileInfo::exists(pathstr::joinPath(workDir, i.key()))) {
            return true;
        }
    }

    return false;
}

// whether the current folder is the working one
QString DataMaintainer::findWorkDir(const VerJson &_json) const
{
    const QString strWorkDir = _json.getInfo(VerJson::h_key_WorkDir);

    // [checking for files in the intended WorkDir]
    const bool isSpecWorkDir = strWorkDir.contains('/')
                               && (isPresentInWorkDir(strWorkDir, _json.data())
                                   || !isPresentInWorkDir(pathstr::parentFolder(_json.file_path()), _json.data()));

    return isSpecWorkDir ? strWorkDir : pathstr::parentFolder(_json.file_path());
}

MetaData DataMaintainer::getMetaData(const VerJson &_json) const
{
    MetaData _meta;
    _meta.dbFilePath = _json.file_path();
    _meta.workDir = findWorkDir(_json);
    _meta.dbFileState = MetaData::Saved;

    // [filter rule]
    FilterMode _filter_mode = FilterMode::NotSet;
    QString _filter_str = _json.getInfo(VerJson::h_key_Ignored);

    if (!_filter_str.isEmpty()) {
        _filter_mode = FilterMode::Ignore;
    } else {
        _filter_str = _json.getInfo(VerJson::h_key_Included);
        if (!_filter_str.isEmpty())
            _filter_mode = FilterMode::Include;
    }

    if (_filter_mode)
        _meta.filter.setFilter(_filter_mode, _filter_str);

    // [algorithm]
    _meta.algorithm = _json.algorithm();

    // [datetime] version 0.4.0+
    QString _strDateTime = _json.getInfo(QStringLiteral(u"time"));

    // compatibility with previous versions
    if (_strDateTime.isEmpty()) {
        const QString _old_upd = _json.getInfo(VerJson::h_key_Updated);
        if (!_old_upd.isEmpty()) {
            _strDateTime = tools::joinStrings(VerJson::h_key_Updated, _old_upd, Lit::s_sepColonSpace);

            const QString _old_verif = _json.getInfo(VerJson::h_key_Verified);
            if (!_old_verif.isEmpty()) {
                _strDateTime += Lit::s_sepCommaSpace;
                _strDateTime += tools::joinStrings(VerJson::h_key_Verified, _old_verif, Lit::s_sepColonSpace);
            }
        }
    }

    _meta.datetime.set(_strDateTime);

    // [flags]
    const QString _strFlags = _json.getInfo(VerJson::h_key_Flags);
    if (_strFlags.contains(QStringLiteral(u"const")))
        _meta.flags |= MetaData::FlagConst;

    return _meta;
}

FileValues DataMaintainer::makeFileValues(const QString &filePath, const QString &basicDate) const
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

TreeModel* DataMaintainer::createDataModel(const VerJson &json, const MetaData &meta)
{
    const QString &workDir = meta.workDir;
    TreeModel *model = new TreeModel();

    // populating the main data
    emit setStatusbarText(QStringLiteral(u"Parsing Json database..."));
    const QString basicDT = considerFileModDate ? meta.datetime.basicDate() : QString();
    const QJsonObject &filelistData = json.data(); // { file_path : checksum }

    for (QJsonObject::const_iterator it = filelistData.constBegin();
         !isCanceled() && it != filelistData.constEnd(); ++it)
    {
        const QString fullPath = pathstr::joinPath(workDir, it.key());
        FileValues values = makeFileValues(fullPath, basicDT);
        values.checksum = it.value().toString();

        model->add_file(it.key(), values);
    }

    // additional data
    QSet<QString> unrCache; // the cache is used when searching for new files

    if (!json.unreadableFiles().isEmpty()) {
        const QJsonArray &unreadableFiles = json.unreadableFiles();

        for (int var = 0; !isCanceled() && var < unreadableFiles.size(); ++var) {
            const QString relPath = unreadableFiles.at(var).toString();
            const QString fullPath = pathstr::joinPath(workDir, relPath);
            const FileStatus status = tools::failedCalcStatus(fullPath);

            if (status & FileStatus::CombUnreadable) {
                model->add_file(relPath, FileValues(status));
                unrCache << relPath;
            }
        }
    }

    // adding new files
    emit setStatusbarText(QStringLiteral(u"Looking for new files..."));
    QDirIterator it(workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        const QString fullPath = it.next();
        const QString relPath = pathstr::relativePath(workDir, fullPath);

        if (meta.filter.isFileAllowed(relPath)
            && !filelistData.contains(relPath)
            && !unrCache.contains(relPath)
            && it.fileInfo().isReadable())
        {
            model->add_file(relPath, FileValues(FileStatus::New, it.fileInfo().size()));
        }
    }

    // end
    emit setStatusbarText();

    if (isCanceled()) {
        qDebug() << "parseJson | Canceled:" << pathstr::basicName(json.file_path());
        delete model;
        return nullptr;
    }

    model->clearCacheFolderItems();
    return model;
}

bool DataMaintainer::importJson(const QString &filePath)
{
    VerJson json(filePath);

    if (!json.load()) {
        emit setStatusbarText("An error occurred while opening the database.");
        return false;
    }

    if (!json) {
        emit showMessage(pathstr::basicName(filePath) + "\n\n"
                                                      "The database doesn't contain checksums.\n"
                                                      "Probably all files have been ignored.", "Empty Database!");
        emit setStatusbarText();
        return false;
    }

    emit setStatusbarText(QStringLiteral(u"Importing Json database..."));

    // parsing
    const MetaData meta = getMetaData(json);         // meta data
    TreeModel *model = createDataModel(json, meta); // main data

    if (!model) // canceled
        return false;

    // setting the parsed data
    DataContainer *parsedData = new DataContainer(meta, model);

    // ToDo: reimplement needed
    const bool success = setSourceData(parsedData);
    if (!success)
        emit failedDataCreation();

    return success;
}

// returns the path to the file if the write was successful, otherwise an empty string
VerJson* DataMaintainer::makeJson(const QModelIndex &rootFolder)
{
    if (!m_data || m_data->m_model->isEmpty()) {
        qDebug() << "makeJson | no data!";
        return nullptr;
    }

    // [Header]
    const bool isBranching = TreeModel::isFolderRow(rootFolder);
    const Numbers &_num = isBranching ? m_data->getNumbers(rootFolder) : m_data->m_numbers;
    const MetaData &_meta = m_data->m_metadata;

    const QString &pathToSave = isBranching ? m_data->branch_path_composed(rootFolder) // branching
                                            : m_data->m_metadata.dbFilePath; // main database

    VerJson *_json = new VerJson(pathToSave);
    _json->addInfo(QStringLiteral(u"Folder"), isBranching ? rootFolder.data().toString() : pathstr::basicName(_meta.workDir));
    _json->addInfo(QStringLiteral(u"Total Size"), format::dataSizeReadableExt(_num.totalSize(FileStatus::CombAvailable)));

    // DateTime
    const QString _dt = (isBranching && m_data->isAllMatched(_num)) ? QStringLiteral(u"Created: ") + format::currentDateTime()
                                                                   : _meta.datetime.toString();
    _json->addInfo(VerJson::h_key_DateTime, _dt);

    // WorkDir
    if (!isBranching && !m_data->isWorkDirRelative())
        _json->addInfo(VerJson::h_key_WorkDir, _meta.workDir);

    // Filter
    if (m_data->isFilterApplied()) {
        const bool _inc = _meta.filter.isFilter(FilterRule::Include);
        const QString &_h_key = _inc ? VerJson::h_key_Included : VerJson::h_key_Ignored;
        _json->addInfo(_h_key, _meta.filter.extensionString());
    }

    // Flags (needs to be redone after expanding the flags list)
    if (_meta.flags)
        _json->addInfo(VerJson::h_key_Flags, QStringLiteral(u"const"));


    // [Main data]
    emit setStatusbarText(QStringLiteral(u"Exporting data to json..."));

    TreeModelIterator iter(m_data->m_model, rootFolder);

    while (iter.hasNext() && !isCanceled()) {
        iter.nextFile();
        const QString checksum = iter.checksum();
        if (!checksum.isEmpty()) {
            _json->addItem(iter.path(rootFolder), checksum);
        }
        else if (iter.status() & FileStatus::CombUnreadable) {
            _json->addItemUnr(iter.path(rootFolder));
        }
    }

    if (isCanceled()) {
        qDebug() << "makeJson: Canceled";
        delete _json;
        return nullptr;
    }

    return _json;
}

bool DataMaintainer::exportToJson()
{
    if (!m_data)
        return false;

    VerJson *_json = makeJson();
    if (!_json)
        return false;

    m_data->makeBackup();

    if (saveJsonFile(_json)) {
        delete _json;
        return true;
    } else {
        // adding the WorkDir info
        _json->addInfo(VerJson::h_key_WorkDir, m_data->m_metadata.workDir);

        // ownership is transferred to the GUI thread (will be deleted there)
        emit failedJsonSave(_json);
        return false;
    }
}

bool DataMaintainer::saveJsonFile(VerJson *_json)
{
    if (!m_data || !_json)
        return false;

    if (_json->save()) {
        setDbFileState(m_data->isInCreation() ? DbFileState::Created : DbFileState::Saved);
        m_data->m_metadata.dbFilePath = _json->file_path();

        emit setStatusbarText(QStringLiteral(u"Saved"));
        qDebug() << "DM::exportToJson >> Saved";

        return true;
    }
    else {
        setDbFileState(DbFileState::NotSaved);

        emit setStatusbarText("NOT Saved");
        qDebug() << "DM::exportToJson >> NOT Saved";

        return false;
    }
}

void DataMaintainer::forkJsonDb(const QModelIndex &rootFolder)
{
    if (!m_data)
        return;

    if (!TreeModel::isFolderRow(rootFolder)) {
        qDebug() << "DM::forkJsonDb | wrong folder index";
        return;
    }

    VerJson *_json = makeJson(rootFolder);

    if (_json && _json->save()) {
        emit subDbForked(_json->file_path());

        // update cached value
        m_data->_cacheBranches[rootFolder] = _json->file_path();
    } else {
        qWarning() << "An error occurred while creating the Branch!";
    }
}

int DataMaintainer::importBranch(const QModelIndex &rootFolder)
{
    if (!m_data)
        return 0;

    const QString _filePath = m_data->branch_path_existing(rootFolder);
    VerJson _json(_filePath);
    if (!_json.load() || !_json)
        return 0;

    if (_json.algorithm() != m_data->m_metadata.algorithm)
        return -1;

    const QJsonObject &_mainList = _json.data();

    int _num = 0;
    TreeModelIterator _it(m_data->m_model, rootFolder);

    while (_it.hasNext()) {
        _it.nextFile();
        if (_it.status() == FileStatus::New) {
            const QJsonValue __v = _mainList.value(_it.path(rootFolder));

            if (!__v.isUndefined()
                && importChecksum(_it.index(), __v.toString()))
            {
                ++_num;
            }
        }
    }

    if (_num) {
        setDbFileState(DbFileState::NotSaved);
        updateNumbers();
        updateDateTime();
    }

    return _num;
}

QString DataMaintainer::itemContentsInfo(const QModelIndex &curIndex)
{
    if (!m_data || !curIndex.isValid() || (curIndex.model() != m_data->m_model))
        return QString();

    if (TreeModel::isFileRow(curIndex)) {
        const QString _fileName = TreeModel::itemName(curIndex);
        const QVariant _fileSize = curIndex.siblingAtColumn(Column::ColumnSize).data(TreeModel::RawDataRole);

        if (_fileSize.isValid()) {
            return format::addStrInParentheses(_fileName,
                                               format::dataSizeReadable(_fileSize.toLongLong()));
        }

        return _fileName;
    }
    // if curIndex is at folder row
    else if (TreeModel::isFolderRow(curIndex)) {
        const Numbers &_num = m_data->getNumbers(curIndex);
        QStringList _sl;

        const NumSize _n_avail = _num.values(FileStatus::CombAvailable);
        if (_n_avail) {
            _sl << QStringLiteral(u"Avail.: ") + format::filesNumSize(_n_avail);
        }

        const NumSize _n_new = _num.values(FileStatus::New);
        if (_n_new) {
            _sl << QStringLiteral(u"New: ") + format::filesNumSize(_n_new);
        }

        const NumSize _n_missing = _num.values(FileStatus::Missing);
        if (_n_missing) {
            _sl << tools::joinStrings(QStringLiteral(u"Missing:"), _n_missing._num);
        }

        const NumSize _n_unr = _num.values(FileStatus::CombUnreadable);
        if (_n_unr) {
            _sl << tools::joinStrings(QStringLiteral(u"Unread.:"), _n_unr._num);
        }

        return _sl.join(QStringLiteral(u"; "));
    }

    return QString();
}

bool DataMaintainer::isCanceled() const
{
    return (proc_ && proc_->isCanceled());
}

bool DataMaintainer::isDataNotSaved() const
{
    return (m_data && m_data->isDbFileState(DbFileState::NotSaved));
}

void DataMaintainer::saveData()
{
    if (isDataNotSaved()) {
        exportToJson();
    }
}

DataMaintainer::~DataMaintainer()
{
    saveData(); // insurance
    qDebug() << Q_FUNC_INFO;
}
