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
    : QObject{parent}, p_view(view), p_settings(settings)
{
    m_icons.setTheme(p_view->palette());
    p_menuAct->setIconTheme(p_view->palette());
    p_menuAct->setSettings(p_settings);

    connectActions();
}

void ModeSelector::connectActions()
{
    // MainWindow menu
    connect(p_menuAct->actionShowFilesystem, SIGNAL(triggered()), this, SLOT(showFileSystem())); // the old syntax is used to apply the default argument
    connect(p_menuAct->actionClearRecent, &QAction::triggered, p_settings, &Settings::clearRecentFiles);
    connect(p_menuAct->actionSave, &QAction::triggered, this, &ModeSelector::saveData);

    // File system View
    connect(p_menuAct->actionToHome, &QAction::triggered, p_view, &View::toHome);
    connect(p_menuAct->actionStop, &QAction::triggered, this, &ModeSelector::stopProcess);
    connect(p_menuAct->actionShowFolderContentsTypes, &QAction::triggered, this, &ModeSelector::showFolderContentTypes);
    connect(p_menuAct->actionProcessChecksumsNoFilter, &QAction::triggered, this, &ModeSelector::processChecksumsNoFilter);
    connect(p_menuAct->actionProcessChecksumsCustomFilter, &QAction::triggered, this, &ModeSelector::processChecksumsFiltered);
    connect(p_menuAct->actionCheckFileByClipboardChecksum, &QAction::triggered, this, &ModeSelector::checkFileByClipboardChecksum);
    connect(p_menuAct->actionProcessSha1File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha1); });
    connect(p_menuAct->actionProcessSha256File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha256); });
    connect(p_menuAct->actionProcessSha512File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha512); });
    connect(p_menuAct->actionProcessSha_toClipboard, &QAction::triggered, this,
            [=]{ processFileSha(p_view->curAbsPath(), p_settings->algorithm(), DestFileProc::Clipboard); });
    connect(p_menuAct->actionOpenDatabase, &QAction::triggered, this, &ModeSelector::doWork);
    connect(p_menuAct->actionCheckSumFile , &QAction::triggered, this, &ModeSelector::doWork);

    // DB Model View
    connect(p_menuAct->actionCancelBackToFS, SIGNAL(triggered()), this, SLOT(showFileSystem()));
    connect(p_menuAct->actionShowDbStatus, &QAction::triggered, p_view, &View::showDbStatus);
    connect(p_menuAct->actionResetDb, &QAction::triggered, this, &ModeSelector::resetDatabase);
    connect(p_menuAct->actionForgetChanges, &QAction::triggered, this, &ModeSelector::restoreDatabase);
    connect(p_menuAct->actionUpdDbReChecksums, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_UpdateMismatches); });
    connect(p_menuAct->actionUpdDbNewLost, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_UpdateNewLost); });
    connect(p_menuAct->actionUpdDbAddNew, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_AddNew); });
    connect(p_menuAct->actionUpdDbClearLost, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_ClearLost); });
    connect(p_menuAct->actionUpdDbFindMoved, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_FindMoved); });
    connect(p_menuAct->actionFilterNewLost, &QAction::triggered, this,
            [=](bool isChecked){ p_view->editFilter(FileStatus::CombNewLost, isChecked); });
    connect(p_menuAct->actionFilterMismatches, &QAction::triggered, this,
            [=](bool isChecked){ p_view->editFilter(FileStatus::Mismatched, isChecked); });
    connect(p_menuAct->actionFilterUnreadable, &QAction::triggered, this,
            [=](bool isChecked){ p_view->editFilter(FileStatus::CombUnreadable, isChecked); });
    connect(p_menuAct->actionFilterModified, &QAction::triggered, this,
            [=](bool isChecked){ p_view->editFilter(FileStatus::NotCheckedMod, isChecked); });
    connect(p_menuAct->actionShowAll, &QAction::triggered, p_view, &View::disableFilter);
    connect(p_menuAct->actionCheckCurFileFromModel, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(p_menuAct->actionCheckItemSubfolder, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(p_menuAct->actionCheckAll, &QAction::triggered, this, &ModeSelector::verifyDb);
    connect(p_menuAct->actionCheckAllMod, &QAction::triggered, this, &ModeSelector::verifyModified);
    connect(p_menuAct->actionCopyStoredChecksum, &QAction::triggered, this, [=]{ copyDataToClipboard(Column::ColumnChecksum); });
    connect(p_menuAct->actionCopyReChecksum, &QAction::triggered, this, [=]{ copyDataToClipboard(Column::ColumnReChecksum); });
    connect(p_menuAct->actionBranchMake, &QAction::triggered, this, &ModeSelector::branchSubfolder);
    connect(p_menuAct->actionBranchOpen, &QAction::triggered, this, &ModeSelector::openBranchDb);
    connect(p_menuAct->actionBranchImport, &QAction::triggered, this, &ModeSelector::importBranch);
    connect(p_menuAct->actionUpdFileAdd, &QAction::triggered, this, &ModeSelector::updateDbItem);
    connect(p_menuAct->actionUpdFileRemove, &QAction::triggered, this, &ModeSelector::updateDbItem);
    connect(p_menuAct->actionUpdFileReChecksum, &QAction::triggered, this, &ModeSelector::updateDbItem);
    connect(p_menuAct->actionExportSum, &QAction::triggered, this, &ModeSelector::exportItemSum);
    connect(p_menuAct->actionUpdFileImportDigest, &QAction::triggered, this, &ModeSelector::importItemSum);
    connect(p_menuAct->actionUpdFilePasteDigest, &QAction::triggered, this, &ModeSelector::pasteItemSum);

    connect(p_menuAct->actionCollapseAll, &QAction::triggered, p_view, &View::collapseAll);
    connect(p_menuAct->actionExpandAll, &QAction::triggered, p_view, &View::expandAll);

    // both
    connect(p_menuAct->actionCopyFile, &QAction::triggered, this, &ModeSelector::copyFsItem);
    connect(p_menuAct->actionCopyFolder, &QAction::triggered, this, &ModeSelector::copyFsItem);

    // Algorithm selection
    connect(p_menuAct->actionSetAlgoSha1, &QAction::triggered, this, [=]{ p_settings->setAlgorithm(QCryptographicHash::Sha1); });
    connect(p_menuAct->actionSetAlgoSha256, &QAction::triggered, this, [=]{ p_settings->setAlgorithm(QCryptographicHash::Sha256); });
    connect(p_menuAct->actionSetAlgoSha512, &QAction::triggered, this, [=]{ p_settings->setAlgorithm(QCryptographicHash::Sha512); });

    // recent files menu
    connect(p_menuAct->menuOpenRecent, &QMenu::triggered, this, &ModeSelector::openRecentDatabase);
}

void ModeSelector::setManager(Manager *manager)
{
    p_manager = manager;
}

void ModeSelector::setProcState(ProcState *procState)
{
    p_proc = procState;
}

void ModeSelector::abortProcess()
{
    if (p_proc->isStarted()) {
        p_manager->clearTasks();
        p_proc->setState(State::Abort);
    }
}

void ModeSelector::stopProcess()
{
    if (p_proc->isStarted()) {
        p_manager->clearTasks();
        p_proc->setState(State::Stop);
    }
}

void ModeSelector::getInfoPathItem()
{
    if (p_proc->isState(State::StartVerbose))
        return;

    if (p_view->isViewFileSystem()) {
        abortProcess();
        p_manager->addTask(&Manager::getPathInfo, p_view->curAbsPath());
    }
    else if (p_view->isViewDatabase()) {
        // info about db item (file or subfolder index)
        p_manager->addTaskWithState(State::Idle,
                                    &Manager::getIndexInfo,
                                    p_view->curIndex());
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
            return format::algoToStr(p_settings->algorithm());
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
            return m_icons.icon(Icons::ProcessStop);
        case DbCreating:
            return m_icons.icon(Icons::ProcessAbort);
        case Folder:
            return m_icons.icon(Icons::FolderSync);
        case File:
            return m_icons.icon(Icons::HashFile);
        case DbFile:
            return m_icons.icon(Icons::Database);
        case SumFile:
            return m_icons.icon(Icons::Scan);
        case Model:
            return m_icons.icon(Icons::Start);
        case ModelNewLost:
            return m_icons.icon(Icons::Update);
        case UpdateMismatch:
            return m_icons.icon(FileStatus::Updated);
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
    if (p_view->isViewDatabase()) {
        if (p_proc && p_proc->isStarted()) {
            if (p_view->data_->isInCreation())
                return DbCreating;
            else if (p_proc->isState(State::StartVerbose))
                return DbProcessing;
        }

        if (!isDbConst()) {
            if (p_view->data_->numbers_.contains(FileStatus::Mismatched))
                return UpdateMismatch;
            else if (p_view->data_->numbers_.contains(FileStatus::CombNewLost))
                return ModelNewLost;
        }

        return Model;
    }

    if (p_view->isViewFileSystem()) {
        if (p_proc && p_proc->isState(State::StartVerbose))
            return FileProcessing;

        QFileInfo pathInfo(p_view->_lastPathFS);
        if (pathInfo.isDir())
            return Folder;
        else if (pathInfo.isFile()) {
            if (paths::isDbFile(p_view->_lastPathFS))
                return DbFile;
            else if (paths::isDigestFile(p_view->_lastPathFS))
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
    processFileSha(p_view->curAbsPath(), algo, DestFileProc::SumFile);
}

void ModeSelector::promptItemFileUpd()
{
    FileStatus storedStatus = TreeModel::itemFileStatus(p_view->curIndex());

    if (!(storedStatus & FileStatus::CombUpdatable))
        return;

    QMessageBox msgBox(p_view);
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);

    switch (storedStatus) {
    case FileStatus::New:
        msgBox.setIconPixmap(m_icons.pixmap(FileStatus::New));
        msgBox.setWindowTitle("New File...");
        msgBox.setText("The database does not yet contain\n"
                       "a corresponding checksum.");
        msgBox.setInformativeText("Would you like to calculate and add it?");
        msgBox.button(QMessageBox::Ok)->setText("Add");
        msgBox.button(QMessageBox::Ok)->setIcon(m_icons.icon(FileStatus::Added));
        break;
    case FileStatus::Missing:
        msgBox.setIconPixmap(m_icons.pixmap(FileStatus::Missing));
        msgBox.setWindowTitle("Missing File...");
        msgBox.setText("File does not exist.");
        msgBox.setInformativeText("Remove the Item from the database?");
        msgBox.button(QMessageBox::Ok)->setText("Remove");
        msgBox.button(QMessageBox::Ok)->setIcon(m_icons.icon(FileStatus::Removed));
        break;
    case FileStatus::Mismatched:
        msgBox.setIconPixmap(m_icons.pixmap(FileStatus::Mismatched));
        msgBox.setWindowTitle("Mismatched Checksum...");
        msgBox.setText("The calculated and stored checksums do not match.");
        msgBox.setInformativeText("Do you want to update the stored one?");
        msgBox.button(QMessageBox::Ok)->setText("Update");
        msgBox.button(QMessageBox::Ok)->setIcon(m_icons.icon(FileStatus::Updated));
        break;
    default:
        break;
    }

    if (msgBox.exec() == QMessageBox::Ok)
        updateDbItem();
}

void ModeSelector::verifyItem()
{
    verify(p_view->curIndex());
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
    if (p_view->isViewFiltered())
        p_view->disableFilter();

    p_manager->addTask(&Manager::updateItemFile,
                      p_view->curIndex(),
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

void ModeSelector::pasteItemSum()
{
    if (!p_view->isViewDatabase())
        return;

    const QString _copied = copiedDigest(p_view->data_->metaData_.algorithm);
    if (!_copied.isEmpty()) {
        p_manager->dataMaintainer->setItemValue(p_view->curIndex(), Column::ColumnChecksum, _copied);
        updateItemFile(DbMod::DM_PasteDigest);
    }
}

void ModeSelector::showFolderContentTypes()
{
    makeFolderContentsList(p_view->curAbsPath());
}

void ModeSelector::checkFileByClipboardChecksum()
{
    checkFile(p_view->curAbsPath(), QGuiApplication::clipboard()->text().simplified());
}

void ModeSelector::copyFsItem()
{
    const QString _itemPath = p_view->curAbsPath();

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
            p_view->_lastPathFS = path;

        if (p_view->data_
            && p_view->data_->isDbFileState(DbFileState::NotSaved))
        {
            p_manager->addTask(&Manager::prepareSwitchToFs);
        }
        else {
            p_view->setFileSystemModel();
        }
    }
}

void ModeSelector::saveData()
{
    p_manager->addTask(&Manager::saveData);
}

void ModeSelector::copyDataToClipboard(Column column)
{
    if (p_view->isViewDatabase()
        && p_view->curIndex().isValid())
    {
        const QString _strData = p_view->curIndex().siblingAtColumn(column).data().toString();
        if (!_strData.isEmpty())
            QGuiApplication::clipboard()->setText(_strData);
    }
}

void ModeSelector::updateDatabase(const DbMod task)
{
    stopProcess();
    p_view->setViewSource();
    p_manager->addTask(&Manager::updateDatabase, task);
}

void ModeSelector::resetDatabase()
{
    if (p_view->data_) {
        p_view->saveHeaderState();
        openJsonDatabase(p_view->data_->metaData_.dbFilePath);
    }
}

void ModeSelector::restoreDatabase()
{
    p_manager->addTask(&Manager::restoreDatabase);
}

void ModeSelector::openJsonDatabase(const QString &filePath)
{
    if (promptProcessAbort()) {
        // aborted process
        if (p_view->isViewDatabase() && p_view->data_->contains(FileStatus::CombProcessing))
            p_view->clear();

        p_manager->addTask(&Manager::saveData);
        p_manager->addTask(&Manager::createDataModel, filePath);
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
    if (!p_view->data_)
        return;

    QString assumedPath = p_view->data_->branch_path_existing(p_view->curIndex());

    if (QFileInfo::exists(assumedPath))
        openJsonDatabase(assumedPath);
}

void ModeSelector::importBranch()
{
    if (!p_view->data_)
        return;

    const QModelIndex _ind = p_view->curIndex();
    const QString _path = p_view->data_->branch_path_existing(_ind);

    if (!_path.isEmpty()) {
        stopProcess();
        p_view->setViewSource();
        p_manager->addTask(&Manager::importBranch, _ind);
    }
}

void ModeSelector::processFileSha(const QString &path, QCryptographicHash::Algorithm algo, DestFileProc result)
{
    p_manager->addTask(&Manager::processFileSha, path, algo, result);
}

void ModeSelector::checkSummaryFile(const QString &path)
{
    p_manager->addTask(&Manager::checkSummaryFile, path);
}

void ModeSelector::checkFile(const QString &filePath, const QString &checkSum)
{
    p_manager->addTask(qOverload<const QString&, const QString&>(&Manager::checkFile), filePath, checkSum);
}

void ModeSelector::verify(const QModelIndex _index)
{
    if (TreeModel::isFileRow(_index)) {
        p_view->disableFilter();
        p_manager->addTask(&Manager::verifyFileItem, _index);
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
    p_view->setViewSource();
    p_manager->addTask(&Manager::verifyFolderItem, _root, _status);
}

void ModeSelector::branchSubfolder()
{
    const QModelIndex _ind = p_view->curIndex();
    if (_ind.isValid()) {
        p_manager->addTask(&Manager::branchSubfolder, _ind);
    }
}

void ModeSelector::exportItemSum()
{
    const QModelIndex _ind = p_view->curIndex();

    if (!p_view->data_ ||
        !TreeModel::hasStatus(FileStatus::CombAvailable, _ind))
    {
        return;
    }

    const QString filePath = p_view->data_->itemAbsolutePath(_ind);

    FileValues fileVal(FileStatus::ToSumFile, QFileInfo(filePath).size());
    fileVal.checksum = TreeModel::hasReChecksum(_ind) ? TreeModel::itemFileReChecksum(_ind)
                                                      : TreeModel::itemFileChecksum(_ind);

    emit p_manager->fileProcessed(filePath, fileVal);
}

void ModeSelector::makeFolderContentsList(const QString &folderPath)
{
    abortProcess();
    p_manager->addTask(&Manager::folderContentsList, folderPath, false);
}

void ModeSelector::makeFolderContentsFilter(const QString &folderPath)
{
    abortProcess();
    p_manager->addTask(&Manager::folderContentsList, folderPath, true);
}

void ModeSelector::_makeDbContentsList()
{
    if (!p_proc->isStarted() && p_view->isViewDatabase()) {
        p_manager->addTask(&Manager::makeDbContentsList);
    }
}

QString ModeSelector::composeDbFilePath()
{
    QString folderName = p_settings->addWorkDirToFilename ? pathstr::basicName(p_view->_lastPathFS) : QString();
    QString _prefix = p_settings->dbPrefix.isEmpty() ? Lit::s_db_prefix : p_settings->dbPrefix;
    QString databaseFileName = format::composeDbFileName(_prefix, folderName, p_settings->dbFileExtension());

    return pathstr::joinPath(p_view->_lastPathFS, databaseFileName);
}

void ModeSelector::processChecksumsFiltered()
{
    //if (isSelectedCreateDb())
    abortProcess();

    if (emptyFolderPrompt())
        makeFolderContentsFilter(p_view->_lastPathFS);
}

void ModeSelector::processChecksumsNoFilter()
{
    if (isSelectedCreateDb()) {
        FilterRule _filter(FilterAttribute::NoAttributes);

        if (p_settings->filter_ignore_db)
            _filter.addAttribute(FilterAttribute::IgnoreDbFiles);
        if (p_settings->filter_ignore_sha)
            _filter.addAttribute(FilterAttribute::IgnoreDigestFiles);
        if (p_settings->filter_ignore_symlinks)
            _filter.addAttribute(FilterAttribute::IgnoreSymlinks);
        if (p_settings->filter_ignore_unpermitted)
            _filter.addAttribute(FilterAttribute::IgnoreUnpermitted);

        processFolderChecksums(_filter);
    }
}

void ModeSelector::processFolderChecksums(const FilterRule &filter)
{
    MetaData metaData;
    metaData.workDir = p_view->_lastPathFS;
    metaData.algorithm = p_settings->algorithm();
    metaData.filter = filter;
    metaData.dbFilePath = composeDbFilePath();
    metaData.dbFileState = DbFileState::NoFile;
    if (p_settings->dbFlagConst)
        metaData.flags |= MetaData::FlagConst;

    p_manager->addTask(&Manager::processFolderSha, metaData);
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

    QMessageBox msgBox(p_view);
    msgBox.setWindowTitle("Existing database detected");
    msgBox.setText("The folder already contains the database file:\n" + pathstr::basicName(dbFilePath));
    msgBox.setInformativeText("Do you want to open or overwrite it?");
    msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::Save | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Open);
    msgBox.setIconPixmap(m_icons.pixmap(Icons::Database));
    msgBox.button(QMessageBox::Save)->setText("Overwrite");
    int ret = msgBox.exec();

    if (ret == QMessageBox::Open)
        openJsonDatabase(dbFilePath);

    return (ret == QMessageBox::Save);
}

bool ModeSelector::emptyFolderPrompt()
{
    if (Files::isEmptyFolder(p_view->_lastPathFS)) {
        QMessageBox messageBox;
        messageBox.information(p_view, "Empty folder", "Nothing to do.");
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
            processFileSha(p_view->_lastPathFS, p_settings->algorithm());
            break;
        case DbFile:
            openJsonDatabase(p_view->_lastPathFS);
            break;
        case SumFile:
            checkSummaryFile(p_view->_lastPathFS);
            break;
        case Model:
            if (!p_proc->isStarted())
                verify();
            break;
        case ModelNewLost:
            if (!p_proc->isStarted())
                updateDatabase(DbMod::DM_UpdateNewLost);
            break;
        case UpdateMismatch:
            if (!p_proc->isStarted())
                updateDatabase(DbMod::DM_UpdateMismatches);
            break;
        case NoMode:
            if (!p_proc->isStarted())
                showFileSystem();
            break;
        default:
            qDebug() << "MainWindow::doWork() | Wrong MODE:" << mode();
            break;
    }
}

void ModeSelector::quickAction()
{
    if (p_proc->isStarted())
            return;

    switch (mode()) {
        case File:
            doWork();
            break;
        case DbFile:
            openJsonDatabase(p_view->_lastPathFS);
            break;
        case SumFile:
            checkSummaryFile(p_view->_lastPathFS);
            break;
        case Model:
        case ModelNewLost:
        case UpdateMismatch:
            if (TreeModel::isFileRow(p_view->curIndex())) {
                if (!isDbConst() && TreeModel::hasStatus(FileStatus::CombUpdatable, p_view->curIndex()))
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
    if (p_view->isViewFileSystem())
        createContextMenu_ViewFs(point);
    else if (p_view->isViewDatabase())
        createContextMenu_ViewDb(point);
    else if (p_view->isViewModel(ModelView::NotSetted))
        p_menuAct->contextMenuViewNot()->exec(p_view->viewport()->mapToGlobal(point));
}

void ModeSelector::createContextMenu_ViewFs(const QPoint &point)
{
    // Filesystem View
    if (!p_view->isViewFileSystem())
        return;

    QModelIndex index = p_view->indexAt(point);
    QMenu *viewContextMenu = p_menuAct->disposableMenu();

    viewContextMenu->addAction(p_menuAct->actionToHome);
    viewContextMenu->addSeparator();

    if (p_proc->isState(State::StartVerbose)) {
        viewContextMenu->addAction(p_menuAct->actionStop);
    }
    else if (index.isValid()) {
        if (isMode(Folder)) {
            viewContextMenu->addAction(p_menuAct->actionShowFolderContentsTypes);
            viewContextMenu->addAction(p_menuAct->actionCopyFolder);
            viewContextMenu->addMenu(p_menuAct->menuAlgorithm(p_settings->algorithm()));
            viewContextMenu->addSeparator();

            viewContextMenu->addAction(p_menuAct->actionProcessChecksumsNoFilter);
            viewContextMenu->addAction(p_menuAct->actionProcessChecksumsCustomFilter);
        }
        else if (isMode(File)) {
            viewContextMenu->addAction(p_menuAct->actionCopyFile);
            viewContextMenu->addAction(p_menuAct->actionProcessSha_toClipboard);
            viewContextMenu->addMenu(p_menuAct->menuAlgorithm(p_settings->algorithm()));
            viewContextMenu->addMenu(p_menuAct->menuCreateDigest);

            const QString _copied = copiedDigest();
            if (!_copied.isEmpty()) {
                QString _s = QStringLiteral(u"Check the file by checksum: ") + format::shortenString(_copied, 20);
                p_menuAct->actionCheckFileByClipboardChecksum->setText(_s);
                viewContextMenu->addSeparator();
                viewContextMenu->addAction(p_menuAct->actionCheckFileByClipboardChecksum);
            }
        }
        else if (isMode(DbFile))
            viewContextMenu->addAction(p_menuAct->actionOpenDatabase);
        else if (isMode(SumFile))
            viewContextMenu->addAction(p_menuAct->actionCheckSumFile);
    }

    viewContextMenu->exec(p_view->viewport()->mapToGlobal(point));
}

void ModeSelector::createContextMenu_ViewDb(const QPoint &point)
{
    // TreeModel or ProxyModel View
    if (!p_view->isViewDatabase())
        return;

    const Numbers &_num = p_view->data_->numbers_;
    const QModelIndex _v_ind = p_view->indexAt(point);
    const QModelIndex index = p_view->isViewModel(ModelView::ModelProxy) ? p_view->data_->proxyModel_->mapToSource(_v_ind) : _v_ind;
    QMenu *viewContextMenu = p_menuAct->disposableMenu();

    if (p_proc->isStarted()) {
        if (!p_view->data_->isInCreation())
            viewContextMenu->addAction(p_menuAct->actionStop);
        viewContextMenu->addAction(p_menuAct->actionCancelBackToFS);
    }
    else {
        viewContextMenu->addAction(p_menuAct->actionShowDbStatus);
        viewContextMenu->addAction(p_menuAct->actionResetDb);

        if (p_view->data_->isDbFileState(DbFileState::NotSaved)
            || QFile::exists(p_view->data_->backupFilePath()))
        {
            viewContextMenu->addAction(p_menuAct->actionForgetChanges);
        }

        viewContextMenu->addSeparator();
        viewContextMenu->addAction(p_menuAct->actionShowFilesystem);
        if (p_view->isViewFiltered())
            viewContextMenu->addAction(p_menuAct->actionShowAll);
        viewContextMenu->addSeparator();

        // filter view
        if (p_view->isViewModel(ModelView::ModelProxy)) {
            if (_num.contains(FileStatus::Mismatched)) {
                p_menuAct->actionFilterMismatches->setChecked(p_view->isViewFiltered(FileStatus::Mismatched));
                viewContextMenu->addAction(p_menuAct->actionFilterMismatches);
            }

            if (_num.contains(FileStatus::CombUnreadable)) {
                p_menuAct->actionFilterUnreadable->setChecked(p_view->isViewFiltered(FileStatus::CombUnreadable));
                viewContextMenu->addAction(p_menuAct->actionFilterUnreadable);
            }

            if (_num.contains(FileStatus::NotCheckedMod)) {
                p_menuAct->actionFilterModified->setChecked(p_view->isViewFiltered(FileStatus::NotCheckedMod));
                viewContextMenu->addAction(p_menuAct->actionFilterModified);
            }

            if (_num.contains(FileStatus::CombNewLost)) {
                p_menuAct->actionFilterNewLost->setChecked(p_view->isViewFiltered(FileStatus::New));
                viewContextMenu->addAction(p_menuAct->actionFilterNewLost);
            }

            if (_num.contains(FileStatus::CombUpdatable))
                viewContextMenu->addSeparator();
        }

        if (!isDbConst() && p_view->data_->hasPossiblyMovedItems())
            viewContextMenu->addAction(p_menuAct->actionUpdDbFindMoved);

        if (index.isValid()) {
            if (TreeModel::isFileRow(index)) {
                // Updatable file item
                if (!isDbConst() && TreeModel::hasStatus(FileStatus::CombUpdatable, index)) {
                    switch (TreeModel::itemFileStatus(index)) {
                        case FileStatus::New:
                            viewContextMenu->addAction(p_menuAct->actionUpdFileAdd);
                            // paste from clipboard
                            if (p_settings->allowPasteIntoDb && !copiedDigest(p_view->data_->metaData_.algorithm).isEmpty())
                                viewContextMenu->addAction(p_menuAct->actionUpdFilePasteDigest);
                            // import from digest file
                            if (QFileInfo::exists(p_view->data_->digestFilePath(index)))
                                viewContextMenu->addAction(p_menuAct->actionUpdFileImportDigest);
                            break;
                        case FileStatus::Missing:
                            viewContextMenu->addAction(p_menuAct->actionUpdFileRemove);
                            break;
                        case FileStatus::Mismatched:
                            viewContextMenu->addAction(p_menuAct->actionUpdFileReChecksum);
                            break;
                        default: break;
                    }
                }

                if (TreeModel::hasReChecksum(index))
                    viewContextMenu->addAction(p_menuAct->actionCopyReChecksum);
                else if (TreeModel::hasChecksum(index))
                    viewContextMenu->addAction(p_menuAct->actionCopyStoredChecksum);

                // Available file item
                if (TreeModel::hasStatus(FileStatus::CombAvailable, index)) {
                    viewContextMenu->addAction(p_menuAct->actionExportSum);
                    viewContextMenu->addAction(p_menuAct->actionCopyFile);

                    viewContextMenu->addAction(p_menuAct->actionCheckCurFileFromModel);
                }
            }
            else { // Folder item
                const bool _has_avail = TreeModel::contains(FileStatus::CombAvailable, index);
                const bool _has_new = TreeModel::contains(FileStatus::New, index);

                if (_has_avail || _has_new) {
                    viewContextMenu->addAction(p_menuAct->actionCopyFolder);

                    // contains branch db file
                    if (!p_view->data_->branch_path_existing(index).isEmpty()) {
                        viewContextMenu->addAction(p_menuAct->actionBranchOpen);
                        if (_has_new && !isDbConst())
                            viewContextMenu->addAction(p_menuAct->actionBranchImport);
                    } else {
                        viewContextMenu->addAction(p_menuAct->actionBranchMake);
                    }

                    if (_has_avail)
                        viewContextMenu->addAction(p_menuAct->actionCheckItemSubfolder);
                }
            }
        }

        if (_num.contains(FileStatus::NotCheckedMod))
            viewContextMenu->addAction(p_menuAct->actionCheckAllMod);

        viewContextMenu->addAction(p_menuAct->actionCheckAll);

        if (!isDbConst() && _num.contains(FileStatus::CombUpdatable))
            viewContextMenu->addMenu(p_menuAct->menuUpdateDb(_num));
    }

    viewContextMenu->addSeparator();
    viewContextMenu->addAction(p_menuAct->actionCollapseAll);
    viewContextMenu->addAction(p_menuAct->actionExpandAll);

    viewContextMenu->exec(p_view->viewport()->mapToGlobal(point));
}

QString ModeSelector::copiedDigest() const
{
    QString clipboardText = QGuiApplication::clipboard()->text().simplified();
    return tools::canBeChecksum(clipboardText) ? clipboardText : QString();
}

QString ModeSelector::copiedDigest(QCryptographicHash::Algorithm algo) const
{
    QString clipboardText = QGuiApplication::clipboard()->text().simplified();
    return tools::canBeChecksum(clipboardText, algo) ? clipboardText : QString();
}

bool ModeSelector::isDbConst() const
{
    return (p_view->data_ && p_view->data_->isImmutable());
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
    if (p_proc->isState(State::StartSilently)) {
        abortProcess();
        return true;
    }

    if (!p_proc->isState(State::StartVerbose))
        return true;

    const QString strAct = abort ? QStringLiteral(u"Abort") : QStringLiteral(u"Stop");
    const QIcon &icoAct = abort ? m_icons.icon(Icons::ProcessAbort) : m_icons.icon(Icons::ProcessStop);

    QMessageBox msgBox(p_view);
    connect(p_proc, &ProcState::progressFinished, &msgBox, &QMessageBox::reject);

    msgBox.setIconPixmap(m_icons.pixmap(FileStatus::Calculating));
    msgBox.setWindowTitle(QStringLiteral(u"Processing..."));
    msgBox.setText(strAct + QStringLiteral(u" current process?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(strAct);
    msgBox.button(QMessageBox::No)->setText(QStringLiteral(u"Continue..."));
    msgBox.button(QMessageBox::Yes)->setIcon(icoAct);
    msgBox.button(QMessageBox::No)->setIcon(m_icons.icon(Icons::DoubleGear));

    if (msgBox.exec() == QMessageBox::Yes) {
        abort ? abortProcess() : stopProcess();
        return true;
    }
    else {
        return false;
    }
}
