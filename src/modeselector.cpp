/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "modeselector.h"
#include <QFileInfo>
#include <QGuiApplication>
#include <QMessageBox>
#include <QClipboard>
#include <QMimeData>
#include <QDebug>
#include <QAbstractButton>
#include "tools.h"
#include "pathstr.h"

ModeSelector::ModeSelector(View *view, Settings *settings, QObject *parent)
    : QObject{parent}, view_(view), settings_(settings)
{
    _icons.setTheme(view_->palette());
    menuAct_->setIconTheme(view_->palette());
    menuAct_->setSettings(settings_);

    connectActions();
}

void ModeSelector::connectActions()
{
    // MainWindow menu
    connect(menuAct_->actionShowFilesystem, SIGNAL(triggered()), this, SLOT(showFileSystem())); // the old syntax is used to apply the default argument
    connect(menuAct_->actionClearRecent, &QAction::triggered, settings_, &Settings::clearRecentFiles);
    connect(menuAct_->actionSave, &QAction::triggered, this, &ModeSelector::saveData);

    // File system View
    connect(menuAct_->actionToHome, &QAction::triggered, view_, &View::toHome);
    connect(menuAct_->actionStop, &QAction::triggered, this, &ModeSelector::stopProcess);
    connect(menuAct_->actionShowFolderContentsTypes, &QAction::triggered, this, &ModeSelector::showFolderContentTypes);
    connect(menuAct_->actionProcessChecksumsNoFilter, &QAction::triggered, this, &ModeSelector::processChecksumsNoFilter);
    connect(menuAct_->actionProcessChecksumsCustomFilter, &QAction::triggered, this, &ModeSelector::processChecksumsFiltered);
    connect(menuAct_->actionCheckFileByClipboardChecksum, &QAction::triggered, this, &ModeSelector::checkFileByClipboardChecksum);
    connect(menuAct_->actionProcessSha1File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha1); });
    connect(menuAct_->actionProcessSha256File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha256); });
    connect(menuAct_->actionProcessSha512File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha512); });
    connect(menuAct_->actionProcessSha_toClipboard, &QAction::triggered, this,
            [=]{ processFileSha(view_->curAbsPath(), settings_->algorithm(), DestFileProc::Clipboard); });
    connect(menuAct_->actionOpenDatabase, &QAction::triggered, this, &ModeSelector::doWork);
    connect(menuAct_->actionCheckSumFile , &QAction::triggered, this, &ModeSelector::doWork);

    // DB Model View
    connect(menuAct_->actionCancelBackToFS, SIGNAL(triggered()), this, SLOT(showFileSystem()));
    connect(menuAct_->actionShowDbStatus, &QAction::triggered, view_, &View::showDbStatus);
    connect(menuAct_->actionResetDb, &QAction::triggered, this, &ModeSelector::resetDatabase);
    connect(menuAct_->actionForgetChanges, &QAction::triggered, this, &ModeSelector::restoreDatabase);
    connect(menuAct_->actionUpdDbReChecksums, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_UpdateMismatches); });
    connect(menuAct_->actionUpdDbNewLost, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_UpdateNewLost); });
    connect(menuAct_->actionUpdDbAddNew, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_AddNew); });
    connect(menuAct_->actionUpdDbClearLost, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_ClearLost); });
    connect(menuAct_->actionUpdDbFindMoved, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_FindMoved); });
    connect(menuAct_->actionFilterNewLost, &QAction::triggered, this,
            [=](bool isChecked){ view_->editFilter(FileStatus::CombNewLost, isChecked); });
    connect(menuAct_->actionFilterMismatches, &QAction::triggered, this,
            [=](bool isChecked){ view_->editFilter(FileStatus::Mismatched, isChecked); });
    connect(menuAct_->actionFilterUnreadable, &QAction::triggered, this,
            [=](bool isChecked){ view_->editFilter(FileStatus::CombUnreadable, isChecked); });
    connect(menuAct_->actionFilterModified, &QAction::triggered, this,
            [=](bool isChecked){ view_->editFilter(FileStatus::NotCheckedMod, isChecked); });
    connect(menuAct_->actionShowAll, &QAction::triggered, view_, &View::disableFilter);
    connect(menuAct_->actionCheckCurFileFromModel, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(menuAct_->actionCheckItemSubfolder, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(menuAct_->actionCheckAll, &QAction::triggered, this, &ModeSelector::verifyDb);
    connect(menuAct_->actionCheckAllMod, &QAction::triggered, this, &ModeSelector::verifyModified);
    connect(menuAct_->actionCopyStoredChecksum, &QAction::triggered, this, [=]{ copyDataToClipboard(Column::ColumnChecksum); });
    connect(menuAct_->actionCopyReChecksum, &QAction::triggered, this, [=]{ copyDataToClipboard(Column::ColumnReChecksum); });
    connect(menuAct_->actionBranchMake, &QAction::triggered, this, &ModeSelector::branchSubfolder);
    connect(menuAct_->actionBranchOpen, &QAction::triggered, this, &ModeSelector::openBranchDb);
    connect(menuAct_->actionBranchImport, &QAction::triggered, this, &ModeSelector::importBranch);
    connect(menuAct_->actionUpdFileAdd, &QAction::triggered, this, &ModeSelector::updateDbItem);
    connect(menuAct_->actionUpdFileRemove, &QAction::triggered, this, &ModeSelector::updateDbItem);
    connect(menuAct_->actionUpdFileReChecksum, &QAction::triggered, this, &ModeSelector::updateDbItem);
    connect(menuAct_->actionExportSum, &QAction::triggered, this, &ModeSelector::exportItemSum);
    connect(menuAct_->actionUpdFileImportDigest, &QAction::triggered, this, &ModeSelector::importItemSum);

    connect(menuAct_->actionCollapseAll, &QAction::triggered, view_, &View::collapseAll);
    connect(menuAct_->actionExpandAll, &QAction::triggered, view_, &View::expandAll);

    // both
    connect(menuAct_->actionCopyFile, &QAction::triggered, this, &ModeSelector::copyFsItem);
    connect(menuAct_->actionCopyFolder, &QAction::triggered, this, &ModeSelector::copyFsItem);

    // Algorithm selection
    connect(menuAct_->actionSetAlgoSha1, &QAction::triggered, this, [=]{ settings_->setAlgorithm(QCryptographicHash::Sha1); });
    connect(menuAct_->actionSetAlgoSha256, &QAction::triggered, this, [=]{ settings_->setAlgorithm(QCryptographicHash::Sha256); });
    connect(menuAct_->actionSetAlgoSha512, &QAction::triggered, this, [=]{ settings_->setAlgorithm(QCryptographicHash::Sha512); });

    // recent files menu
    connect(menuAct_->menuOpenRecent, &QMenu::triggered, this, &ModeSelector::openRecentDatabase);
}

void ModeSelector::setManager(Manager *manager)
{
    manager_ = manager;
}

void ModeSelector::setProcState(ProcState *procState)
{
    proc_ = procState;
}

void ModeSelector::abortProcess()
{
    if (proc_->isStarted()) {
        manager_->clearTasks();
        proc_->setState(State::Abort);
    }
}

void ModeSelector::stopProcess()
{
    if (proc_->isStarted()) {
        manager_->clearTasks();
        proc_->setState(State::Stop);
    }
}

void ModeSelector::getInfoPathItem()
{
    if (proc_->isState(State::StartVerbose))
        return;

    if (view_->isViewFileSystem()) {
        abortProcess();
        manager_->addTask(&Manager::getPathInfo, view_->curAbsPath());
    }
    else if (view_->isViewDatabase()) {
        // info about db item (file or subfolder index)
        manager_->addTaskWithState(State::Idle,
                                   &Manager::getIndexInfo,
                                   view_->curIndex());
    }
}

QString ModeSelector::getButtonText()
{
    switch (mode()) {
        case FileProcessing:
        case DbProcessing:
            return QStringLiteral(u"Stop");
        case DbCreating:
            return QStringLiteral(u"Abort");
        case Folder:
        case File:
            return format::algoToStr(settings_->algorithm());
        case DbFile:
            return QStringLiteral(u"Open");
        case SumFile:
            return QStringLiteral(u"Check");
        case Model:
            return QStringLiteral(u"Verify");
        case ModelNewLost:
            return QStringLiteral(u"New/Lost");
        case UpdateMismatch:
            return QStringLiteral(u"Update");
        case NoMode:
            return QStringLiteral(u"Browse");
        default:
            break;
    }

    return "?";
}

QIcon ModeSelector::getButtonIcon()
{
    switch (mode()) {
        case FileProcessing:
        case DbProcessing:
            return _icons.icon(Icons::ProcessStop);
        case DbCreating:
            return _icons.icon(Icons::ProcessAbort);
        case Folder:
            return _icons.icon(Icons::FolderSync);
        case File:
            return _icons.icon(Icons::HashFile);
        case DbFile:
            return _icons.icon(Icons::Database);
        case SumFile:
            return _icons.icon(Icons::Scan);
        case Model:
            return _icons.icon(Icons::Start);
        case ModelNewLost:
            return _icons.icon(Icons::Update);
        case UpdateMismatch:
            return _icons.icon(FileStatus::Updated);
        default:
            break;
    }

    return QIcon();
}

QString ModeSelector::getButtonToolTip()
{
    switch (mode()) {
        case Folder:
            return QStringLiteral(u"Calculate checksums of contained files\n"
                                  "and save the result to the local database");
        case File:
            return QStringLiteral(u"Calculate file Digest");
        case Model:
            return QStringLiteral(u"Check ALL files against stored checksums");
        case ModelNewLost:
            return QStringLiteral(u"Update the Database:\nadd new files, delete missing ones");
        case UpdateMismatch:
            return QStringLiteral(u"Update mismatched checksums with newly calculated ones");
        default:
            break;
    }

    return QString();
}

Mode ModeSelector::mode() const
{
    if (view_->isViewDatabase()) {
        if (proc_ && proc_->isStarted()) {
            if (view_->data_->isInCreation())
                return DbCreating;
            else if (proc_->isState(State::StartVerbose))
                return DbProcessing;
        }

        if (!isDbConst()) {
            if (view_->data_->numbers_.contains(FileStatus::Mismatched))
                return UpdateMismatch;
            else if (view_->data_->numbers_.contains(FileStatus::CombNewLost))
                return ModelNewLost;
        }

        return Model;
    }

    if (view_->isViewFileSystem()) {
        if (proc_ && proc_->isState(State::StartVerbose))
            return FileProcessing;

        QFileInfo pathInfo(view_->_lastPathFS);
        if (pathInfo.isDir())
            return Folder;
        else if (pathInfo.isFile()) {
            if (paths::isDbFile(view_->_lastPathFS))
                return DbFile;
            else if (paths::isDigestFile(view_->_lastPathFS))
                return SumFile;
            else
                return File;
        }
    }

    return NoMode;
}

bool ModeSelector::isMode(const Modes expected)
{
    return (expected & mode());
}

// tasks execution --->>>
void ModeSelector::procSumFile(QCryptographicHash::Algorithm algo)
{
    processFileSha(view_->curAbsPath(), algo, DestFileProc::SumFile);
}

void ModeSelector::promptItemFileUpd()
{
    FileStatus storedStatus = TreeModel::itemFileStatus(view_->curIndex());

    if (!(storedStatus & FileStatus::CombUpdatable))
        return;

    QMessageBox msgBox(view_);
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);

    switch (storedStatus) {
    case FileStatus::New:
        msgBox.setIconPixmap(_icons.pixmap(FileStatus::New));
        msgBox.setWindowTitle("New File...");
        msgBox.setText("The database does not yet contain\n"
                       "a corresponding checksum.");
        msgBox.setInformativeText("Would you like to calculate and add it?");
        msgBox.button(QMessageBox::Ok)->setText("Add");
        msgBox.button(QMessageBox::Ok)->setIcon(_icons.icon(FileStatus::Added));
        break;
    case FileStatus::Missing:
        msgBox.setIconPixmap(_icons.pixmap(FileStatus::Missing));
        msgBox.setWindowTitle("Missing File...");
        msgBox.setText("File does not exist.");
        msgBox.setInformativeText("Remove the Item from the database?");
        msgBox.button(QMessageBox::Ok)->setText("Remove");
        msgBox.button(QMessageBox::Ok)->setIcon(_icons.icon(FileStatus::Removed));
        break;
    case FileStatus::Mismatched:
        msgBox.setIconPixmap(_icons.pixmap(FileStatus::Mismatched));
        msgBox.setWindowTitle("Mismatched Checksum...");
        msgBox.setText("The calculated and stored checksums do not match.");
        msgBox.setInformativeText("Do you want to update the stored one?");
        msgBox.button(QMessageBox::Ok)->setText("Update");
        msgBox.button(QMessageBox::Ok)->setIcon(_icons.icon(FileStatus::Updated));
        break;
    default:
        break;
    }

    if (msgBox.exec() == QMessageBox::Ok)
        updateDbItem();
}

void ModeSelector::verifyItem()
{
    verify(view_->curIndex());
}

void ModeSelector::verifyDb()
{
    verify();
}

void ModeSelector::updateItemFile(DbMod _job)
{
    /*  At the moment, updating a separate file
     *  with the View filtering enabled is unstable,
     *  so the filter should be disabled before making changes.
     */
    if (view_->isViewFiltered())
        view_->disableFilter();

    manager_->addTask(&Manager::updateItemFile,
                      view_->curIndex(),
                      _job);
}

void ModeSelector::updateDbItem()
{
    updateItemFile(DbMod::DM_AutoSelect);
}

void ModeSelector::importItemSum()
{
    updateItemFile(DbMod::DM_ImportDigest);
}

void ModeSelector::showFolderContentTypes()
{
    makeFolderContentsList(view_->curAbsPath());
}

void ModeSelector::checkFileByClipboardChecksum()
{
    checkFile(view_->curAbsPath(), QGuiApplication::clipboard()->text().simplified());
}

void ModeSelector::copyFsItem()
{
    const QString _itemPath = view_->curAbsPath();

    if (!_itemPath.isEmpty()) {
        QMimeData* mimeData = new QMimeData();
        mimeData->setUrls({ QUrl::fromLocalFile(_itemPath) });
        QGuiApplication::clipboard()->setMimeData(mimeData);
    }
}

void ModeSelector::showFileSystem(const QString &path)
{
    if (promptProcessAbort()) {
        if (QFileInfo::exists(path))
            view_->_lastPathFS = path;

        if (view_->data_
            && view_->data_->isDbFileState(DbFileState::NotSaved))
        {
            manager_->addTask(&Manager::prepareSwitchToFs);
        }
        else {
            view_->setFileSystemModel();
        }
    }
}

void ModeSelector::saveData()
{
    manager_->addTask(&Manager::saveData);
}

void ModeSelector::copyDataToClipboard(Column column)
{
    if (view_->isViewDatabase()
        && view_->curIndex().isValid())
    {
        const QString _strData = view_->curIndex().siblingAtColumn(column).data().toString();
        if (!_strData.isEmpty())
            QGuiApplication::clipboard()->setText(_strData);
    }
}

void ModeSelector::updateDatabase(const DbMod task)
{
    stopProcess();
    view_->setViewSource();
    manager_->addTask(&Manager::updateDatabase, task);
}

void ModeSelector::resetDatabase()
{
    if (view_->data_) {
        view_->saveHeaderState();
        openJsonDatabase(view_->data_->metaData_.dbFilePath);
    }
}

void ModeSelector::restoreDatabase()
{
    manager_->addTask(&Manager::restoreDatabase);
}

void ModeSelector::openJsonDatabase(const QString &filePath)
{
    if (promptProcessAbort()) {
        // aborted process
        if (view_->isViewDatabase() && view_->data_->contains(FileStatus::CombProcessing))
            view_->clear();

        manager_->addTask(&Manager::saveData);
        manager_->addTask(&Manager::createDataModel, filePath);
    }
}

void ModeSelector::openRecentDatabase(const QAction *action)
{
    // the recent files menu stores the path to the DB file in the action's tooltip
    QString filePath = action->toolTip();

    if (QFileInfo::exists(filePath))
        openJsonDatabase(filePath);
}

void ModeSelector::openBranchDb()
{
    if (!view_->data_)
        return;

    QString assumedPath = view_->data_->branch_path_existing(view_->curIndex());

    if (QFileInfo::exists(assumedPath))
        openJsonDatabase(assumedPath);
}

void ModeSelector::importBranch()
{
    if (!view_->data_)
        return;

    const QModelIndex _ind = view_->curIndex();
    const QString _path = view_->data_->branch_path_existing(_ind);

    if (!_path.isEmpty()) {
        stopProcess();
        view_->setViewSource();
        manager_->addTask(&Manager::importBranch, _ind);
    }
}

void ModeSelector::processFileSha(const QString &path, QCryptographicHash::Algorithm algo, DestFileProc result)
{
    manager_->addTask(&Manager::processFileSha, path, algo, result);
}

void ModeSelector::checkSummaryFile(const QString &path)
{
    manager_->addTask(&Manager::checkSummaryFile, path);
}

void ModeSelector::checkFile(const QString &filePath, const QString &checkSum)
{
    manager_->addTask(qOverload<const QString&, const QString&>(&Manager::checkFile), filePath, checkSum);
}

void ModeSelector::verify(const QModelIndex _index)
{
    if (TreeModel::isFileRow(_index)) {
        view_->disableFilter();
        manager_->addTask(&Manager::verifyFileItem, _index);
    }
    else {
        verifyItems(_index, FileStatus::CombNotChecked);
    }
}

void ModeSelector::verifyModified()
{
    verifyItems(QModelIndex(), FileStatus::NotCheckedMod);
}

void ModeSelector::verifyItems(const QModelIndex &_root, FileStatus _status)
{
    stopProcess();
    view_->setViewSource();
    manager_->addTask(&Manager::verifyFolderItem, _root, _status);
}

void ModeSelector::branchSubfolder()
{
    const QModelIndex _ind = view_->curIndex();
    if (_ind.isValid()) {
        manager_->addTask(&Manager::branchSubfolder, _ind);
    }
}

void ModeSelector::exportItemSum()
{
    const QModelIndex _ind = view_->curIndex();

    if (!view_->data_ ||
        !TreeModel::hasStatus(FileStatus::CombAvailable, _ind))
    {
        return;
    }

    const QString filePath = view_->data_->itemAbsolutePath(_ind);

    FileValues fileVal(FileStatus::ToSumFile, QFileInfo(filePath).size());
    fileVal.checksum = TreeModel::hasReChecksum(_ind) ? TreeModel::itemFileReChecksum(_ind)
                                                      : TreeModel::itemFileChecksum(_ind);

    emit manager_->fileProcessed(filePath, fileVal);
}

void ModeSelector::makeFolderContentsList(const QString &folderPath)
{
    abortProcess();
    manager_->addTask(&Manager::folderContentsList, folderPath, false);
}

void ModeSelector::makeFolderContentsFilter(const QString &folderPath)
{
    abortProcess();
    manager_->addTask(&Manager::folderContentsList, folderPath, true);
}

void ModeSelector::_makeDbContentsList()
{
    if (!proc_->isStarted() && view_->isViewDatabase()) {
        manager_->addTask(&Manager::makeDbContentsList);
    }
}

QString ModeSelector::composeDbFilePath()
{
    QString folderName = settings_->addWorkDirToFilename ? pathstr::basicName(view_->_lastPathFS) : QString();
    QString _prefix = settings_->dbPrefix.isEmpty() ? Lit::s_db_prefix : settings_->dbPrefix;
    QString databaseFileName = format::composeDbFileName(_prefix, folderName, settings_->dbFileExtension());

    return pathstr::joinPath(view_->_lastPathFS, databaseFileName);
}

void ModeSelector::processChecksumsFiltered()
{
    //if (isSelectedCreateDb())
    abortProcess();

    if (emptyFolderPrompt())
        makeFolderContentsFilter(view_->_lastPathFS);
}

void ModeSelector::processChecksumsNoFilter()
{
    if (isSelectedCreateDb()) {

        //_filter.ignoreDbFiles = settings_->filter_ignore_db;
        //_filter.ignoreShaFiles = settings_->filter_ignore_sha;

        quint8 _attr = 0;

        if (settings_->filter_ignore_db)
            _attr |= FilterAttribute::IgnoreDbFiles;
        if (settings_->filter_ignore_sha)
            _attr |= FilterAttribute::IgnoreDigestFiles;
        if (settings_->excludeSymlinks)
            _attr |= FilterAttribute::IgnoreSymlinks;
        if (settings_->excludeUnpermitted)
            _attr |= FilterAttribute::IgnoreUnpermitted;

        FilterRule _filter((FilterAttribute)_attr);

        processFolderChecksums(_filter);
    }
}

void ModeSelector::processFolderChecksums(const FilterRule &filter)
{
    MetaData metaData;
    metaData.workDir = view_->_lastPathFS;
    metaData.algorithm = settings_->algorithm();
    metaData.filter = filter;
    metaData.dbFilePath = composeDbFilePath();
    metaData.dbFileState = DbFileState::NoFile;
    if (settings_->dbFlagConst)
        metaData.flags |= MetaData::FlagConst;

    manager_->addTask(&Manager::processFolderSha, metaData);
}

bool ModeSelector::isSelectedCreateDb()
{
    /* if a very large folder is selected,
     * the file system iteration (info about folder contents process) may continue for some time,
     * so cancelation is needed before starting a new process
     */

    abortProcess();

    return (emptyFolderPrompt() && overwriteDbPrompt());
}

bool ModeSelector::overwriteDbPrompt()
{
    const QString dbFilePath = composeDbFilePath();

    if (!QFileInfo(dbFilePath).isFile())
        return true;

    QMessageBox msgBox(view_);
    msgBox.setWindowTitle("Existing database detected");
    msgBox.setText("The folder already contains the database file:\n" + pathstr::basicName(dbFilePath));
    msgBox.setInformativeText("Do you want to open or overwrite it?");
    msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::Save | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Open);
    msgBox.setIconPixmap(_icons.pixmap(Icons::Database));
    msgBox.button(QMessageBox::Save)->setText("Overwrite");
    int ret = msgBox.exec();

    if (ret == QMessageBox::Open)
        openJsonDatabase(dbFilePath);

    return (ret == QMessageBox::Save);
}

bool ModeSelector::emptyFolderPrompt()
{
    if (Files::isEmptyFolder(view_->_lastPathFS)) {
        QMessageBox messageBox;
        messageBox.information(view_, "Empty folder", "Nothing to do.");
        return false;
    }

    return true;
}

void ModeSelector::doWork()
{
    switch (mode()) {
        case FileProcessing:
        case DbProcessing:
            stopProcess();
            break;
        case DbCreating:
            showFileSystem();
            break;
        case Folder:
            processChecksumsFiltered();
            break;
        case File:
            processFileSha(view_->_lastPathFS, settings_->algorithm());
            break;
        case DbFile:
            openJsonDatabase(view_->_lastPathFS);
            break;
        case SumFile:
            checkSummaryFile(view_->_lastPathFS);
            break;
        case Model:
            if (!proc_->isStarted())
                verify();
            break;
        case ModelNewLost:
            if (!proc_->isStarted())
                updateDatabase(DbMod::DM_UpdateNewLost);
            break;
        case UpdateMismatch:
            if (!proc_->isStarted())
                updateDatabase(DbMod::DM_UpdateMismatches);
            break;
        case NoMode:
            if (!proc_->isStarted())
                showFileSystem();
            break;
        default:
            qDebug() << "MainWindow::doWork() | Wrong MODE:" << mode();
            break;
    }
}

void ModeSelector::quickAction()
{
    if (proc_->isStarted())
            return;

    switch (mode()) {
        case File:
            doWork();
            break;
        case DbFile:
            openJsonDatabase(view_->_lastPathFS);
            break;
        case SumFile:
            checkSummaryFile(view_->_lastPathFS);
            break;
        case Model:
        case ModelNewLost:
        case UpdateMismatch:
            if (TreeModel::isFileRow(view_->curIndex())) {
                if (!isDbConst() && TreeModel::hasStatus(FileStatus::CombUpdatable, view_->curIndex()))
                    promptItemFileUpd();
                else
                    verifyItem();
            }
            break;
        default:
            break;
    }
}

void ModeSelector::createContextMenu_View(const QPoint &point)
{
    if (view_->isViewFileSystem())
        createContextMenu_ViewFs(point);
    else if (view_->isViewDatabase())
        createContextMenu_ViewDb(point);
    else if (view_->isViewModel(ModelView::NotSetted))
        menuAct_->contextMenuViewNot()->exec(view_->viewport()->mapToGlobal(point));
}

void ModeSelector::createContextMenu_ViewFs(const QPoint &point)
{
    // Filesystem View
    if (!view_->isViewFileSystem())
        return;

    QModelIndex index = view_->indexAt(point);
    QMenu *viewContextMenu = menuAct_->disposableMenu();

    viewContextMenu->addAction(menuAct_->actionToHome);
    viewContextMenu->addSeparator();

    if (proc_->isState(State::StartVerbose)) {
        viewContextMenu->addAction(menuAct_->actionStop);
    }
    else if (index.isValid()) {
        if (isMode(Folder)) {
            viewContextMenu->addAction(menuAct_->actionShowFolderContentsTypes);
            viewContextMenu->addAction(menuAct_->actionCopyFolder);
            viewContextMenu->addMenu(menuAct_->menuAlgorithm(settings_->algorithm()));
            viewContextMenu->addSeparator();

            viewContextMenu->addAction(menuAct_->actionProcessChecksumsNoFilter);
            viewContextMenu->addAction(menuAct_->actionProcessChecksumsCustomFilter);
        }
        else if (isMode(File)) {
            viewContextMenu->addAction(menuAct_->actionCopyFile);
            viewContextMenu->addAction(menuAct_->actionProcessSha_toClipboard);
            viewContextMenu->addMenu(menuAct_->menuAlgorithm(settings_->algorithm()));
            viewContextMenu->addMenu(menuAct_->menuCreateDigest);

            QString clipboardText = QGuiApplication::clipboard()->text().simplified();
            if (tools::canBeChecksum(clipboardText)) {
                QString _s = QStringLiteral(u"Check the file by checksum: ") + format::shortenString(clipboardText, 20);
                menuAct_->actionCheckFileByClipboardChecksum->setText(_s);
                viewContextMenu->addSeparator();
                viewContextMenu->addAction(menuAct_->actionCheckFileByClipboardChecksum);
            }
        }
        else if (isMode(DbFile))
            viewContextMenu->addAction(menuAct_->actionOpenDatabase);
        else if (isMode(SumFile))
            viewContextMenu->addAction(menuAct_->actionCheckSumFile);
    }

    viewContextMenu->exec(view_->viewport()->mapToGlobal(point));
}

void ModeSelector::createContextMenu_ViewDb(const QPoint &point)
{
    // TreeModel or ProxyModel View
    if (!view_->isViewDatabase())
        return;

    const Numbers &_num = view_->data_->numbers_;
    const QModelIndex _v_ind = view_->indexAt(point);
    const QModelIndex index = view_->isViewModel(ModelView::ModelProxy) ? view_->data_->proxyModel_->mapToSource(_v_ind) : _v_ind;
    QMenu *viewContextMenu = menuAct_->disposableMenu();

    if (proc_->isStarted()) {
        if (!view_->data_->isInCreation())
            viewContextMenu->addAction(menuAct_->actionStop);
        viewContextMenu->addAction(menuAct_->actionCancelBackToFS);
    }
    else {
        viewContextMenu->addAction(menuAct_->actionShowDbStatus);
        viewContextMenu->addAction(menuAct_->actionResetDb);

        if (view_->data_->isDbFileState(DbFileState::NotSaved)
            || QFile::exists(view_->data_->backupFilePath()))
        {
            viewContextMenu->addAction(menuAct_->actionForgetChanges);
        }

        viewContextMenu->addSeparator();
        viewContextMenu->addAction(menuAct_->actionShowFilesystem);
        if (view_->isViewFiltered())
            viewContextMenu->addAction(menuAct_->actionShowAll);
        viewContextMenu->addSeparator();

        // filter view
        if (view_->isViewModel(ModelView::ModelProxy)) {
            if (_num.contains(FileStatus::Mismatched)) {
                menuAct_->actionFilterMismatches->setChecked(view_->isViewFiltered(FileStatus::Mismatched));
                viewContextMenu->addAction(menuAct_->actionFilterMismatches);
            }

            if (_num.contains(FileStatus::CombUnreadable)) {
                menuAct_->actionFilterUnreadable->setChecked(view_->isViewFiltered(FileStatus::CombUnreadable));
                viewContextMenu->addAction(menuAct_->actionFilterUnreadable);
            }

            if (_num.contains(FileStatus::NotCheckedMod)) {
                menuAct_->actionFilterModified->setChecked(view_->isViewFiltered(FileStatus::NotCheckedMod));
                viewContextMenu->addAction(menuAct_->actionFilterModified);
            }

            if (_num.contains(FileStatus::CombNewLost)) {
                menuAct_->actionFilterNewLost->setChecked(view_->isViewFiltered(FileStatus::New));
                viewContextMenu->addAction(menuAct_->actionFilterNewLost);
            }

            if (_num.contains(FileStatus::CombUpdatable))
                viewContextMenu->addSeparator();
        }

        if (!isDbConst() && view_->data_->hasPossiblyMovedItems())
            viewContextMenu->addAction(menuAct_->actionUpdDbFindMoved);

        if (index.isValid()) {
            if (TreeModel::isFileRow(index)) {
                // Updatable file item
                if (!isDbConst() && TreeModel::hasStatus(FileStatus::CombUpdatable, index)) {
                    switch (TreeModel::itemFileStatus(index)) {
                        case FileStatus::New:
                            viewContextMenu->addAction(menuAct_->actionUpdFileAdd);
                            if (QFileInfo::exists(view_->data_->digestFilePath(index)))
                                viewContextMenu->addAction(menuAct_->actionUpdFileImportDigest);
                            break;
                        case FileStatus::Missing:
                            viewContextMenu->addAction(menuAct_->actionUpdFileRemove);
                            break;
                        case FileStatus::Mismatched:
                            viewContextMenu->addAction(menuAct_->actionUpdFileReChecksum);
                            break;
                        default: break;
                    }
                }

                if (TreeModel::hasReChecksum(index))
                    viewContextMenu->addAction(menuAct_->actionCopyReChecksum);
                else if (TreeModel::hasChecksum(index))
                    viewContextMenu->addAction(menuAct_->actionCopyStoredChecksum);

                // Available file item
                if (TreeModel::hasStatus(FileStatus::CombAvailable, index)) {
                    viewContextMenu->addAction(menuAct_->actionExportSum);
                    viewContextMenu->addAction(menuAct_->actionCopyFile);

                    viewContextMenu->addAction(menuAct_->actionCheckCurFileFromModel);
                }
            }
            else { // Folder item
                const bool _has_avail = TreeModel::contains(FileStatus::CombAvailable, index);
                const bool _has_new = TreeModel::contains(FileStatus::New, index);

                if (_has_avail || _has_new) {
                    viewContextMenu->addAction(menuAct_->actionCopyFolder);

                    // contains branch db file
                    if (!view_->data_->branch_path_existing(index).isEmpty()) {
                        viewContextMenu->addAction(menuAct_->actionBranchOpen);
                        if (_has_new && !isDbConst())
                            viewContextMenu->addAction(menuAct_->actionBranchImport);
                    } else {
                        viewContextMenu->addAction(menuAct_->actionBranchMake);
                    }

                    if (_has_avail)
                        viewContextMenu->addAction(menuAct_->actionCheckItemSubfolder);
                }
            }
        }

        if (_num.contains(FileStatus::NotCheckedMod))
            viewContextMenu->addAction(menuAct_->actionCheckAllMod);

        viewContextMenu->addAction(menuAct_->actionCheckAll);

        if (!isDbConst() && _num.contains(FileStatus::CombUpdatable))
            viewContextMenu->addMenu(menuAct_->menuUpdateDb(_num));
    }

    viewContextMenu->addSeparator();
    viewContextMenu->addAction(menuAct_->actionCollapseAll);
    viewContextMenu->addAction(menuAct_->actionExpandAll);

    viewContextMenu->exec(view_->viewport()->mapToGlobal(point));
}

bool ModeSelector::isDbConst() const
{
    return (view_->data_ && view_->data_->isImmutable());
}

bool ModeSelector::promptProcessStop()
{
    return promptMessageProcCancelation_(false);
}

bool ModeSelector::promptProcessAbort()
{
    return promptMessageProcCancelation_(true);
}

bool ModeSelector::promptMessageProcCancelation_(bool abort)
{
    if (proc_->isState(State::StartSilently)) {
        abortProcess();
        return true;
    }

    if (!proc_->isState(State::StartVerbose))
        return true;

    const QString strAct = abort ? QStringLiteral(u"Abort") : QStringLiteral(u"Stop");
    const QIcon &icoAct = abort ? _icons.icon(Icons::ProcessAbort) : _icons.icon(Icons::ProcessStop);

    QMessageBox msgBox(view_);
    connect(proc_, &ProcState::progressFinished, &msgBox, &QMessageBox::reject);

    msgBox.setIconPixmap(_icons.pixmap(FileStatus::Calculating));
    msgBox.setWindowTitle(QStringLiteral(u"Processing..."));
    msgBox.setText(strAct + QStringLiteral(u" current process?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(strAct);
    msgBox.button(QMessageBox::No)->setText(QStringLiteral(u"Continue..."));
    msgBox.button(QMessageBox::Yes)->setIcon(icoAct);
    msgBox.button(QMessageBox::No)->setIcon(_icons.icon(Icons::DoubleGear));

    if (msgBox.exec() == QMessageBox::Yes) {
        abort ? abortProcess() : stopProcess();
        return true;
    }
    else {
        return false;
    }
}
