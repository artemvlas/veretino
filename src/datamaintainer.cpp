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

DataMaintainer::DataMaintainer(DataContainer *initData, QObject *parent)
    : QObject(parent)
{
    connections();
    setData(initData);
}

void DataMaintainer::connections()
{
    connect(this, &DataMaintainer::numbersUpdated, this, [=]{ if (mData->contains(FileStatus::Mismatched | FileStatus::Missing))
                                                                  mData->m_metadata.datetime.m_verified.clear(); });
}

void DataMaintainer::setProcState(const ProcState *procState)
{
    mProc = procState;
}

void DataMaintainer::setData()
{
    setData(new DataContainer(this));
}

void DataMaintainer::setData(const MetaData &meta, TreeModel *dataModel)
{
    setData(new DataContainer(meta, dataModel, this));
}

bool DataMaintainer::setData(DataContainer *sourceData)
{
    if (sourceData) {
        clearOldData();
        mOldData = mData;
        mData = sourceData;
        mData->setParent(this);
        updateNumbers();
    }

    return sourceData; // false if sourceData == nullptr, else true
}

void DataMaintainer::clearData()
{
    if (mData) {
        delete mData;
        mData = nullptr;
    }
}

void DataMaintainer::clearOldData()
{
    if (mOldData) {
        delete mOldData;
        mOldData = nullptr;
    }
}

bool DataMaintainer::setItemValue(const QModelIndex &fileIndex, Column column, const QVariant &value)
{
    return (mData && mData->m_model->setData(fileIndex.siblingAtColumn(column), value));
}

void DataMaintainer::setFileStatus(const QModelIndex &index, FileStatus status)
{
    setItemValue(index, Column::ColumnStatus, status);
}

void DataMaintainer::setConsiderDateModified(bool consider)
{
    m_considerFileModDate = consider;
}

void DataMaintainer::updateDateTime()
{
    if (!mData)
        return;

    VerDateTime &dt = mData->m_metadata.datetime;

    if (mData->isDbFileState(DbFileState::NoFile)) {
        dt.update(VerDateTime::Created);
    }
    else if (mData->contains(FileStatus::CombDbChanged)) {
        dt.update(VerDateTime::Updated);
    }
}

void DataMaintainer::updateVerifDateTime()
{
    if (mData && mData->isAllMatched()) {
        mData->m_metadata.datetime.update(VerDateTime::Verified);
        setDbFileState(DbFileState::NotSaved);
    }
}

// iterate the 'metaData.workDir' folder and add files to the data model
int DataMaintainer::setFolderBasedData(const MetaData &meta, FileStatus fileStatus)
{
    const QString &workDir = meta.workDir;
    const FilterRule &filter = meta.filter;

    if (!QFileInfo(workDir).isDir()) {
        qWarning() << "DM::setFolderBasedData | Wrong path:" << workDir;
        return 0;
    }

    emit setStatusbarText(QStringLiteral(u"Creating a list of files..."));

    int numAdded = 0;
    TreeModel *model = new TreeModel;

    QDirIterator it(workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        const QString fullPath = it.next();
        const QString relPath = pathstr::relativePath(workDir, fullPath);
        const QFileInfo &fi = it.fileInfo();

        if (filter.isAllowed(fi)) {
            const FileValues &values = fi.isReadable() ? FileValues(fileStatus, fi.size())
                                                       : FileValues(FileStatus::UnPermitted);

            model->add_file(relPath, values);
            ++numAdded;
        }
    }

    if (isCanceled() || numAdded == 0) {
        qDebug() << "DM::setFolderBasedData | Canceled/No items:" << workDir;
        emit setStatusbarText();
        emit failedDataCreation();
        delete model;
        return 0;
    }

    model->clearCacheFolderItems();

    setData(meta, model);

    return numAdded;
}

void DataMaintainer::updateNumbers()
{
    if (!mData) {
        qDebug() << "DM::updateNumbers | NO mData";
        return;
    }

    mData->updateNumbers();
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
    if (mData
        && mData->m_numbers.moveFile(status_old, status_new, size))
    {
        emit numbersUpdated();
    }
}

void DataMaintainer::moveNumbers(const FileStatus before, const FileStatus after)
{
    if (mData && mData->m_numbers.changeStatus(before, after))
        emit numbersUpdated();
}

void DataMaintainer::setDbFileState(DbFileState state)
{
    if (mData && !mData->isDbFileState(state)) {
        mData->m_metadata.dbFileState = state;
        emit dbFileStateChanged(state);
    }
}

bool DataMaintainer::updateChecksum(const QModelIndex &fileRowIndex, const QString &computedChecksum)
{
    if (!mData || !fileRowIndex.isValid() || fileRowIndex.model() != mData->m_model) {
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

        // mData->numbers_.moveFile(FileStatus::New, FileStatus::Imported, TreeModel::itemFileSize(_file));
    }

    return false;
}

int DataMaintainer::changeStatuses(const FileStatuses statuses, const FileStatus newStatus, const QModelIndex &rootIndex)
{
    if (!mData || tools::isFlagCombined(newStatus)) {
        qDebug() << "DM::changeStatuses >> Error";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(mData->m_model, rootIndex);

    while (iter.hasNext()) {
        if (statuses & iter.nextFile().status()) {
            setFileStatus(iter.index(), newStatus);
            ++number;
        }
    }

    if (number > 0)
        updateNumbers();

    return number;
}

int DataMaintainer::addToQueue(const FileStatuses statuses, const QModelIndex &rootIndex)
{
    return changeStatuses(statuses, FileStatus::Queued, rootIndex);
}

void DataMaintainer::clearChecksum(const QModelIndex &fileIndex)
{
    setItemValue(fileIndex, Column::ColumnChecksum);
}

int DataMaintainer::clearChecksums(const FileStatuses statuses, const QModelIndex &rootIndex)
{
    if (!mData) {
        qDebug() << "DM::clearChecksums | NO mData";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(mData->m_model, rootIndex);

    while (iter.hasNext()) {
        if (statuses & iter.nextFile().status()) {
            setItemValue(iter.index(), Column::ColumnChecksum);
            ++number;
        }
    }

    return number;
}

int DataMaintainer::clearLostFiles()
{
    if (!mData) {
        qDebug() << "DM::clearLostFiles | NO mData";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(mData->m_model);

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
    if (mData && TreeModel::hasStatus(FileStatus::Missing, fileIndex)) {
        clearChecksum(fileIndex);
        setFileStatus(fileIndex, FileStatus::Removed);
        return true;
    }

    return false;
}

bool DataMaintainer::tryMoved(const QModelIndex &file, const QString &checksum)
{
    if (!mData || !mData->_cacheMissing.contains(checksum))
        return false;

    // moved out
    QModelIndex ind_movedout = mData->_cacheMissing.take(checksum);
    const FileStatus status = TreeModel::itemFileStatus(ind_movedout);

    if (status & (FileStatus::Missing | FileStatus::Removed)) {
        clearChecksum(ind_movedout);
        setFileStatus(ind_movedout, FileStatus::MovedOut);
        mData->m_numbers.moveFile(status, FileStatus::MovedOut);

        // moved
        setFileStatus(file, FileStatus::Moved);
        setItemValue(file, Column::ColumnChecksum, checksum);
        return true;
    }

    return false;
}

int DataMaintainer::updateMismatchedChecksums()
{
    if (!mData) {
        qDebug() << "DM::updateMismatchedChecksums | NO mData";
        return 0;
    }

    int number = 0;
    TreeModelIterator iter(mData->m_model);

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
    if (mData && TreeModel::hasReChecksum(fileIndex)) {
        const QString reChecksum = TreeModel::itemFileReChecksum(fileIndex);

        if (!reChecksum.isEmpty()) {
            setItemValue(fileIndex, Column::ColumnChecksum, reChecksum);
            setItemValue(fileIndex, Column::ColumnReChecksum);
            setItemValue(fileIndex, Column::ColumnStatus, FileStatus::Updated);
            return true;
        }
    }

    return false;
}

void DataMaintainer::rollBackStoppedCalc(const QModelIndex &rootIndex, FileStatus prevStatus)
{
    if (prevStatus != FileStatus::Queued) { // mData && !mData->isInCreation()
        if (prevStatus == FileStatus::New)
            clearChecksums(FileStatus::Added, rootIndex);
        else if (tools::isFlagCombined(prevStatus)) // CombNotChecked == NotChecked | NotCheckedMod
            prevStatus = FileStatus::NotChecked;

        changeStatuses((FileStatus::CombProcessing | FileStatus::Added), prevStatus, rootIndex);
    }
}

// checks if there is at least one file from the list (json.data() keys) in the folder (workDir)
bool DataMaintainer::isPresentInWorkDir(const VerJson &json, const QString &workDir) const
{
    if (!QFileInfo::exists(workDir))
        return false;

    const QJsonObject &fileList = json.data();

    QJsonObject::const_iterator i;

    for (i = fileList.constBegin(); !isCanceled() && i != fileList.constEnd(); ++i) {
        if (QFileInfo::exists(pathstr::joinPath(workDir, i.key()))) {
            return true;
        }
    }

    return false;
}

// whether the current folder is the working one
QString DataMaintainer::findWorkDir(const VerJson &json) const
{
    const QString strWorkDir = json.getInfo(VerJson::h_key_WorkDir);

    // [checking for files in the intended WorkDir]
    const bool isSpecWorkDir = strWorkDir.contains('/')
                               && (isPresentInWorkDir(json, strWorkDir)
                                   || !isPresentInWorkDir(json, pathstr::parentFolder(json.file_path())));

    return isSpecWorkDir ? strWorkDir : pathstr::parentFolder(json.file_path());
}

MetaData DataMaintainer::getMetaData(const VerJson &json) const
{
    MetaData meta;
    meta.dbFilePath = json.file_path();
    meta.workDir = findWorkDir(json);
    meta.dbFileState = MetaData::Saved;

    // [filter rule]
    FilterMode filter_mode = FilterMode::NotSet;
    QString filter_str = json.getInfo(VerJson::h_key_Ignored);

    if (!filter_str.isEmpty()) {
        filter_mode = FilterMode::Ignore;
    } else {
        filter_str = json.getInfo(VerJson::h_key_Included);
        if (!filter_str.isEmpty())
            filter_mode = FilterMode::Include;
    }

    if (filter_mode)
        meta.filter.setFilter(filter_mode, filter_str);

    // [algorithm]
    meta.algorithm = json.algorithm();

    // [datetime] version 0.4.0+
    QString strDateTime = json.getInfo(QStringLiteral(u"time"));

    // compatibility with previous versions
    if (strDateTime.isEmpty()) {
        const QString old_upd = json.getInfo(VerJson::h_key_Updated);
        if (!old_upd.isEmpty()) {
            strDateTime = tools::joinStrings(VerJson::h_key_Updated, old_upd, Lit::s_sepColonSpace);

            const QString old_verif = json.getInfo(VerJson::h_key_Verified);
            if (!old_verif.isEmpty()) {
                strDateTime += Lit::s_sepCommaSpace;
                strDateTime += tools::joinStrings(VerJson::h_key_Verified, old_verif, Lit::s_sepColonSpace);
            }
        }
    }

    meta.datetime.set(strDateTime);

    // [flags]
    const QString strFlags = json.getInfo(VerJson::h_key_Flags);
    if (strFlags.contains(QStringLiteral(u"const")))
        meta.flags |= MetaData::FlagConst;

    // [comment]
    meta.comment = json.getInfo(VerJson::h_key_Comment);

    return meta;
}

TreeModel* DataMaintainer::createDataModel(const VerJson &json, const MetaData &meta)
{
    auto makeFileValues = [](const QString &filePath, const QString &basicDate) {
        if (!QFileInfo::exists(filePath))
            return FileValues(FileStatus::Missing);

        QFileInfo fi(filePath);
        FileStatus status = (!basicDate.isEmpty()
                             && tools::isLater(basicDate, fi.lastModified()))
                                ? FileStatus::NotCheckedMod : FileStatus::NotChecked;

        return FileValues(status, fi.size());
    };

    const QString &workDir = meta.workDir;
    TreeModel *pModel = new TreeModel();

    // populating the main data
    emit setStatusbarText(QStringLiteral(u"Parsing Json database..."));
    const QString basicDate = m_considerFileModDate ? meta.datetime.basicDate() : QString();
    const QJsonObject &itemList = json.data(); // { file_path : checksum }

    for (QJsonObject::const_iterator it = itemList.constBegin();
         !isCanceled() && it != itemList.constEnd(); ++it)
    {
        const QString fullPath = pathstr::joinPath(workDir, it.key());
        FileValues values = makeFileValues(fullPath, basicDate);
        values.checksum = it.value().toString();

        pModel->add_file(it.key(), values);
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
                pModel->add_file(relPath, FileValues(status));
                unrCache << relPath;
            }
        }
    }

    // adding new files
    emit setStatusbarText(QStringLiteral(u"Looking for new files..."));
    QDirIterator it(workDir, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        const QString relPath = pathstr::relativePath(workDir, it.next());
        const QFileInfo &fi = it.fileInfo();

        if (meta.filter.isAllowed(fi)
            && !itemList.contains(relPath)
            && !unrCache.contains(relPath))
        {
            pModel->add_file(relPath, FileValues(FileStatus::New, fi.size()));
        }
    }

    // end
    emit setStatusbarText();

    if (isCanceled()) {
        qDebug() << "parseJson | Canceled:" << pathstr::basicName(json.file_path());
        delete pModel;
        return nullptr;
    }

    pModel->clearCacheFolderItems();
    return pModel;
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
    TreeModel *model = createDataModel(json, meta);  // main data

    if (!model) { // canceled
        emit failedDataCreation();
        return false;
    }

    // setting the parsed data
    setData(meta, model);
    return true;
}

// returns the path to the file if the write was successful, otherwise an empty string
VerJson* DataMaintainer::makeJson(const QModelIndex &rootFolder)
{
    if (!mData || mData->m_model->isEmpty()) {
        qDebug() << "makeJson | no data!";
        return nullptr;
    }

    /*** Header ***/
    const bool isBranching = TreeModel::isFolderRow(rootFolder);
    const Numbers &nums = isBranching ? mData->getNumbers(rootFolder) : mData->m_numbers;
    const MetaData &meta = mData->m_metadata;

    const QString &pathToSave = isBranching ? mData->branch_path_composed(rootFolder) // branching
                                            : mData->m_metadata.dbFilePath;           // main database

    VerJson *pJson = new VerJson(pathToSave);
    pJson->addInfo(QStringLiteral(u"Folder"), isBranching ? rootFolder.data().toString() : pathstr::basicName(meta.workDir));
    pJson->addInfo(QStringLiteral(u"Total Size"), format::dataSizeReadableExt(nums.totalSize(FileStatus::CombAvailable)));

    // DateTime
    const QString timestamp = (isBranching && mData->isAllMatched(nums)) ? VerDateTime::current(VerDateTime::Created)
                                                                          : meta.datetime.toString();
    pJson->addInfo(VerJson::h_key_DateTime, timestamp);

    // WorkDir
    if (!isBranching && !mData->isWorkDirRelative())
        pJson->addInfo(VerJson::h_key_WorkDir, meta.workDir);

    // Filter
    if (mData->isFilterApplied()) {
        const bool inc = meta.filter.isFilter(FilterRule::Include);
        const QString &h_key = inc ? VerJson::h_key_Included : VerJson::h_key_Ignored;
        pJson->addInfo(h_key, meta.filter.extensionString());
    }

    // Flags (needs to be reimplement after expanding the flags list)
    if (meta.flags)
        pJson->addInfo(VerJson::h_key_Flags, QStringLiteral(u"const"));

    // Comment
    if (!meta.comment.isEmpty())
        pJson->addInfo(VerJson::h_key_Comment, meta.comment);

    /*** Main data ***/
    emit setStatusbarText(QStringLiteral(u"Exporting data to json..."));

    TreeModelIterator iter(mData->m_model, rootFolder);

    while (iter.hasNext() && !isCanceled()) {
        iter.nextFile();
        const QString checksum = iter.checksum();
        if (!checksum.isEmpty()) {
            pJson->addItem(iter.path(rootFolder), checksum);
        }
        else if (iter.status() & FileStatus::CombUnreadable) {
            pJson->addItemUnr(iter.path(rootFolder));
        }
    }

    if (isCanceled()) {
        qDebug() << "makeJson: Canceled";
        delete pJson;
        return nullptr;
    }

    return pJson;
}

bool DataMaintainer::exportToJson()
{
    if (!mData)
        return false;

    VerJson *pJson = makeJson();
    if (!pJson)
        return false;

    mData->makeBackup();

    if (saveJsonFile(pJson)) {
        delete pJson;
        return true;
    } else {
        // adding the WorkDir info
        pJson->addInfo(VerJson::h_key_WorkDir, mData->m_metadata.workDir);

        // ownership is transferred to the GUI thread (will be deleted there)
        emit failedJsonSave(pJson);
        return false;
    }
}

bool DataMaintainer::saveJsonFile(VerJson *pJson)
{
    if (!mData || !pJson)
        return false;

    if (pJson->save()) {
        setDbFileState(mData->isInCreation() ? DbFileState::Created : DbFileState::Saved);
        mData->m_metadata.dbFilePath = pJson->file_path();

        emit setStatusbarText(QStringLiteral(u"Saved"));
        qDebug() << "DM::exportToJson >> Saved";

        return true;
    } else {
        setDbFileState(DbFileState::NotSaved);

        emit setStatusbarText("NOT Saved");
        qDebug() << "DM::exportToJson >> NOT Saved";

        return false;
    }
}

void DataMaintainer::forkJsonDb(const QModelIndex &rootFolder)
{
    if (!mData)
        return;

    if (!TreeModel::isFolderRow(rootFolder)) {
        qDebug() << "DM::forkJsonDb | wrong folder index";
        return;
    }

    VerJson *pJson = makeJson(rootFolder);

    if (pJson && pJson->save()) {
        emit subDbForked(pJson->file_path());

        // update cached value
        mData->_cacheBranches[rootFolder] = pJson->file_path();
    } else {
        qWarning() << "An error occurred while creating the Branch!";
    }
}

int DataMaintainer::importBranch(const QModelIndex &rootFolder)
{
    if (!mData)
        return 0;

    const QString filePath = mData->branch_path_existing(rootFolder);
    VerJson json(filePath);
    if (!json.load() || !json)
        return 0;

    if (json.algorithm() != mData->m_metadata.algorithm)
        return -1;

    const QJsonObject &mainList = json.data();

    int num = 0;
    TreeModelIterator it(mData->m_model, rootFolder);

    while (it.hasNext()) {
        it.nextFile();
        if (it.status() == FileStatus::New) {
            const QJsonValue val = mainList.value(it.path(rootFolder));

            if (!val.isUndefined()
                && importChecksum(it.index(), val.toString()))
            {
                ++num;
            }
        }
    }

    if (num) {
        setDbFileState(DbFileState::NotSaved);
        updateNumbers();
        updateDateTime();
    }

    return num;
}

QString DataMaintainer::itemContentsInfo(const QModelIndex &curIndex)
{
    if (!mData || !curIndex.isValid() || (curIndex.model() != mData->m_model))
        return QString();

    if (TreeModel::isFileRow(curIndex)) {
        const QString fileName = TreeModel::itemName(curIndex);
        const QVariant fileSize = curIndex.siblingAtColumn(Column::ColumnSize).data(TreeModel::RawDataRole);

        if (fileSize.isValid()) {
            return format::addStrInParentheses(fileName,
                                               format::dataSizeReadable(fileSize.toLongLong()));
        }

        return fileName;
    }
    // if curIndex is at folder row
    else if (TreeModel::isFolderRow(curIndex)) {
        const Numbers &num = mData->getNumbers(curIndex);

        QList<FileStatus> statuses = num.statuses();

        std::sort(statuses.begin(), statuses.end(),
                  [&num](const FileStatus &t1, const FileStatus &t2) {
            return num.values(t1) > num.values(t2);
        }
        );

        QStringList sl;

        for (const FileStatus status : statuses) {
            if (status == FileStatus::MovedOut && statuses.contains(FileStatus::Moved))
                continue;

            const QString statusName = format::fileItemStatus(status);
            const NumSize numSize = num.values(status);
            QString str;

            if (numSize._size > 0)
                str = tools::joinStrings(statusName, format::filesNumSize(numSize), Lit::s_sepColonSpace);
            else
                str = tools::joinStrings(statusName, numSize._num);

            sl << str;
        }

        return sl.join(Lit::s_sepCommaSpace);
    }

    return QString();
}

bool DataMaintainer::isCanceled() const
{
    return (mProc && mProc->isCanceled());
}

bool DataMaintainer::isDataNotSaved() const
{
    return (mData && mData->isDbFileState(DbFileState::NotSaved));
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
