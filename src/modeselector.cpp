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
    m_menuAct->setIconTheme(p_view->palette());
    m_menuAct->setSettings(p_settings);

    connectActions();
}

void ModeSelector::connectActions()
{
    // MainWindow menu
    connect(m_menuAct->actionShowFilesystem, SIGNAL(triggered()), this, SLOT(showFileSystem())); // the old syntax is used to apply the default argument
    connect(m_menuAct->actionClearRecent, &QAction::triggered, p_settings, &Settings::clearRecentFiles);
    connect(m_menuAct->actionSave, &QAction::triggered, this, &ModeSelector::saveData);

    // File system View
    connect(m_menuAct->actionToHome, &QAction::triggered, p_view, &View::toHome);
    connect(m_menuAct->actionStop, &QAction::triggered, this, &ModeSelector::stopProcess);
    connect(m_menuAct->actionShowFolderContentsTypes, &QAction::triggered, this, &ModeSelector::showFolderContentTypes);
    connect(m_menuAct->actionProcessChecksumsNoFilter, &QAction::triggered, this, &ModeSelector::processChecksumsNoFilter);
    connect(m_menuAct->actionProcessChecksumsCustomFilter, &QAction::triggered, this, &ModeSelector::processChecksumsFiltered);
    connect(m_menuAct->actionCheckFileByClipboardChecksum, &QAction::triggered, this, &ModeSelector::checkFileByClipboardChecksum);
    connect(m_menuAct->actionProcessSha1File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha1); });
    connect(m_menuAct->actionProcessSha256File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha256); });
    connect(m_menuAct->actionProcessSha512File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha512); });
    connect(m_menuAct->actionProcessSha_toClipboard, &QAction::triggered, this,
            [=]{ processFileSha(p_view->curAbsPath(), p_settings->algorithm(), DestFileProc::Clipboard); });
    connect(m_menuAct->actionOpenDatabase, &QAction::triggered, this, &ModeSelector::doWork);
    connect(m_menuAct->actionCheckSumFile , &QAction::triggered, this, &ModeSelector::doWork);

    // DB Model View
    connect(m_menuAct->actionCancelBackToFS, SIGNAL(triggered()), this, SLOT(showFileSystem()));
    connect(m_menuAct->actionShowDbStatus, &QAction::triggered, p_view, &View::showDbStatus);
    connect(m_menuAct->actionResetDb, &QAction::triggered, this, &ModeSelector::resetDatabase);
    connect(m_menuAct->actionForgetChanges, &QAction::triggered, this, &ModeSelector::restoreDatabase);
    connect(m_menuAct->actionUpdDbReChecksums, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_UpdateMismatches); });
    connect(m_menuAct->actionUpdDbNewLost, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_UpdateNewLost); });
    connect(m_menuAct->actionUpdDbAddNew, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_AddNew); });
    connect(m_menuAct->actionUpdDbClearLost, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_ClearLost); });
    connect(m_menuAct->actionUpdDbFindMoved, &QAction::triggered, this, [=]{ updateDatabase(DbMod::DM_FindMoved); });
    connect(m_menuAct->actionFilterNewLost, &QAction::triggered, this,
            [=](bool isChecked){ p_view->editFilter(FileStatus::CombNewLost, isChecked); });
    connect(m_menuAct->actionFilterMismatches, &QAction::triggered, this,
            [=](bool isChecked){ p_view->editFilter(FileStatus::Mismatched, isChecked); });
    connect(m_menuAct->actionFilterUnreadable, &QAction::triggered, this,
            [=](bool isChecked){ p_view->editFilter(FileStatus::CombUnreadable, isChecked); });
    connect(m_menuAct->actionFilterModified, &QAction::triggered, this,
            [=](bool isChecked){ p_view->editFilter(FileStatus::NotCheckedMod, isChecked); });
    connect(m_menuAct->actionShowAll, &QAction::triggered, p_view, &View::disableFilter);
    connect(m_menuAct->actionCheckCurFileFromModel, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(m_menuAct->actionCheckItemSubfolder, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(m_menuAct->actionCheckAll, &QAction::triggered, this, &ModeSelector::verifyDb);
    connect(m_menuAct->actionCheckAllMod, &QAction::triggered, this, &ModeSelector::verifyModified);
    connect(m_menuAct->actionCopyStoredChecksum, &QAction::triggered, this, [=]{ copyDataToClipboard(Column::ColumnChecksum); });
    connect(m_menuAct->actionCopyReChecksum, &QAction::triggered, this, [=]{ copyDataToClipboard(Column::ColumnReChecksum); });
    connect(m_menuAct->actionBranchMake, &QAction::triggered, this, &ModeSelector::branchSubfolder);
    connect(m_menuAct->actionBranchOpen, &QAction::triggered, this, &ModeSelector::openBranchDb);
    connect(m_menuAct->actionBranchImport, &QAction::triggered, this, &ModeSelector::importBranch);
    connect(m_menuAct->actionUpdFileAdd, &QAction::triggered, this, &ModeSelector::updateDbItem);
    connect(m_menuAct->actionUpdFileRemove, &QAction::triggered, this, &ModeSelector::updateDbItem);
    connect(m_menuAct->actionUpdFileReChecksum, &QAction::triggered, this, &ModeSelector::updateDbItem);
    connect(m_menuAct->actionExportSum, &QAction::triggered, this, &ModeSelector::exportItemSum);
    connect(m_menuAct->actionUpdFileImportDigest, &QAction::triggered, this, &ModeSelector::importItemSum);
    connect(m_menuAct->actionUpdFilePasteDigest, &QAction::triggered, this, &ModeSelector::pasteItemSum);

    connect(m_menuAct->actionCollapseAll, &QAction::triggered, p_view, &View::collapseAll);
    connect(m_menuAct->actionExpandAll, &QAction::triggered, p_view, &View::expandAll);

    // both
    connect(m_menuAct->actionCopyFile, &QAction::triggered, this, &ModeSelector::copyFsItem);
    connect(m_menuAct->actionCopyFolder, &QAction::triggered, this, &ModeSelector::copyFsItem);

    // Algorithm selection
    connect(m_menuAct->actionSetAlgoSha1, &QAction::triggered, this, [=]{ p_settings->setAlgorithm(QCryptographicHash::Sha1); });
    connect(m_menuAct->actionSetAlgoSha256, &QAction::triggered, this, [=]{ p_settings->setAlgorithm(QCryptographicHash::Sha256); });
    connect(m_menuAct->actionSetAlgoSha512, &QAction::triggered, this, [=]{ p_settings->setAlgorithm(QCryptographicHash::Sha512); });

    // recent files menu
    connect(m_menuAct->menuOpenRecent, &QMenu::triggered, this, &ModeSelector::openRecentDatabase);
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
            if (p_view->m_data->isInCreation())
                return DbCreating;
            else if (p_proc->isState(State::StartVerbose))
                return DbProcessing;
        }

        if (!isDbConst()) {
            if (p_view->m_data->m_numbers.contains(FileStatus::Mismatched))
                return UpdateMismatch;
            else if (p_view->m_data->m_numbers.contains(FileStatus::CombNewLost))
                return ModelNewLost;
        }

        return Model;
    }

    if (p_view->isViewFileSystem()) {
        if (p_proc && p_proc->isState(State::StartVerbose))
            return FileProcessing;

        QFileInfo pathInfo(p_view->m_lastPathFS);
        if (pathInfo.isDir())
            return Folder;
        else if (pathInfo.isFile()) {
            if (paths::isDbFile(p_view->m_lastPathFS))
                return DbFile;
            else if (paths::isDigestFile(p_view->m_lastPathFS))
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

void ModeSelector::updateItemFile(DbMod job)
{
    /*  At the moment, updating a separate file
     *  with the View filtering enabled is unstable,
     *  so the filter should be disabled before making changes.
     */
    if (p_view->isViewFiltered())
        p_view->disableFilter();

    p_manager->addTask(&Manager::updateItemFile,
                      p_view->curIndex(),
                      job);
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

    const QString copied = copiedDigest(p_view->m_data->m_metadata.algorithm);
    if (!copied.isEmpty()) {
        p_manager->m_dataMaintainer->setItemValue(p_view->curIndex(), Column::ColumnChecksum, copied);
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
    const QString itemPath = p_view->curAbsPath();

    if (!itemPath.isEmpty()) {
        QMimeData* mimeData = new QMimeData();
        mimeData->setUrls({ QUrl::fromLocalFile(itemPath) });
        QGuiApplication::clipboard()->setMimeData(mimeData);
    }
}

void ModeSelector::showFileSystem(const QString &path)
{
    if (!promptProcessAbort())
        return;

    if (QFileInfo::exists(path))
        p_view->m_lastPathFS = path;

    if (p_view->m_data
        && p_view->m_data->isDbFileState(DbFileState::NotSaved))
    {
        p_manager->addTask(&Manager::prepareSwitchToFs);
        p_proc->setAwaiting(ProcState::AwaitingSwitchToFs);
    } else {
        p_view->setFileSystemModel();
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
        const QString strData = p_view->curIndex().siblingAtColumn(column).data().toString();
        if (!strData.isEmpty())
            QGuiApplication::clipboard()->setText(strData);
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
    if (p_view->m_data) {
        p_view->saveHeaderState();
        openJsonDatabase(p_view->m_data->m_metadata.dbFilePath);
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
        if (p_view->isViewDatabase() && p_view->m_data->contains(FileStatus::CombProcessing))
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
    if (!p_view->m_data)
        return;

    QString assumedPath = p_view->m_data->branch_path_existing(p_view->curIndex());

    if (QFileInfo::exists(assumedPath))
        openJsonDatabase(assumedPath);
}

void ModeSelector::importBranch()
{
    if (!p_view->m_data)
        return;

    const QModelIndex ind = p_view->curIndex();
    const QString path = p_view->m_data->branch_path_existing(ind);

    if (!path.isEmpty()) {
        stopProcess();
        p_view->setViewSource();
        p_manager->addTask(&Manager::importBranch, ind);
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

void ModeSelector::verify(const QModelIndex index)
{
    if (TreeModel::isFileRow(index)) {
        p_view->disableFilter();
        p_manager->addTask(&Manager::verifyFileItem, index);
    } else {
        verifyItems(index, FileStatus::CombNotChecked);
    }
}

void ModeSelector::verifyModified()
{
    verifyItems(QModelIndex(), FileStatus::NotCheckedMod);
}

void ModeSelector::verifyItems(const QModelIndex &root, FileStatus status)
{
    stopProcess();
    p_view->setViewSource();
    p_manager->addTask(&Manager::verifyFolderItem, root, status);
}

void ModeSelector::branchSubfolder()
{
    const QModelIndex ind = p_view->curIndex();
    if (ind.isValid()) {
        p_manager->addTask(&Manager::branchSubfolder, ind);
    }
}

void ModeSelector::exportItemSum()
{
    const QModelIndex ind = p_view->curIndex();

    if (!p_view->m_data ||
        !TreeModel::hasStatus(FileStatus::CombAvailable, ind))
    {
        return;
    }

    const QString filePath = p_view->m_data->itemAbsolutePath(ind);

    FileValues fileVal(FileStatus::ToSumFile, QFileInfo(filePath).size());
    fileVal.checksum = TreeModel::hasReChecksum(ind) ? TreeModel::itemFileReChecksum(ind)
                                                      : TreeModel::itemFileChecksum(ind);

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

void ModeSelector::makeDbContList()
{
    if (!p_proc->isStarted() && p_view->isViewDatabase()) {
        p_manager->addTask(&Manager::makeDbContentsList);
    }
}

QString ModeSelector::composeDbFilePath()
{
    QString folderName = p_settings->addWorkDirToFilename ? pathstr::basicName(p_view->m_lastPathFS) : QString();
    QString prefix = p_settings->dbPrefix.isEmpty() ? Lit::s_db_prefix : p_settings->dbPrefix;
    QString databaseFileName = format::composeDbFileName(prefix, folderName, p_settings->dbFileExtension());

    return pathstr::joinPath(p_view->m_lastPathFS, databaseFileName);
}

void ModeSelector::processChecksumsFiltered()
{
    //if (isSelectedCreateDb())
    abortProcess();

    if (emptyFolderPrompt())
        makeFolderContentsFilter(p_view->m_lastPathFS);
}

void ModeSelector::processChecksumsNoFilter()
{
    if (isSelectedCreateDb()) {
        FilterRule filter(FilterAttribute::NoAttributes);

        if (p_settings->filter_ignore_db)
            filter.addAttribute(FilterAttribute::IgnoreDbFiles);
        if (p_settings->filter_ignore_sha)
            filter.addAttribute(FilterAttribute::IgnoreDigestFiles);
        if (p_settings->filter_ignore_symlinks)
            filter.addAttribute(FilterAttribute::IgnoreSymlinks);
        if (p_settings->filter_ignore_unpermitted)
            filter.addAttribute(FilterAttribute::IgnoreUnpermitted);

        processFolderChecksums(filter);
    }
}

void ModeSelector::processFolderChecksums(const FilterRule &filter, const QString comment)
{
    MetaData metaData;
    metaData.workDir = p_view->m_lastPathFS;
    metaData.algorithm = p_settings->algorithm();
    metaData.filter = filter;
    metaData.dbFilePath = composeDbFilePath();
    metaData.dbFileState = DbFileState::NoFile;
    metaData.comment = comment;

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
    if (Files::isEmptyFolder(p_view->m_lastPathFS)) {
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
            processFileSha(p_view->m_lastPathFS, p_settings->algorithm());
            break;
        case DbFile:
            openJsonDatabase(p_view->m_lastPathFS);
            break;
        case SumFile:
            checkSummaryFile(p_view->m_lastPathFS);
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
            openJsonDatabase(p_view->m_lastPathFS);
            break;
        case SumFile:
            checkSummaryFile(p_view->m_lastPathFS);
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
        m_menuAct->contextMenuViewNot()->exec(p_view->viewport()->mapToGlobal(point));
}

void ModeSelector::createContextMenu_ViewFs(const QPoint &point)
{
    // Filesystem View
    if (!p_view->isViewFileSystem())
        return;

    QModelIndex index = p_view->indexAt(point);
    QMenu *viewContextMenu = m_menuAct->disposableMenu();

    viewContextMenu->addAction(m_menuAct->actionToHome);
    viewContextMenu->addSeparator();

    if (p_proc->isState(State::StartVerbose)) {
        viewContextMenu->addAction(m_menuAct->actionStop);
    } else if (index.isValid()) {
        if (isMode(Folder)) {
            viewContextMenu->addAction(m_menuAct->actionShowFolderContentsTypes);
            viewContextMenu->addAction(m_menuAct->actionCopyFolder);
            viewContextMenu->addMenu(m_menuAct->menuAlgorithm(p_settings->algorithm()));
            viewContextMenu->addSeparator();

            viewContextMenu->addAction(m_menuAct->actionProcessChecksumsNoFilter);
            viewContextMenu->addAction(m_menuAct->actionProcessChecksumsCustomFilter);
        } else if (isMode(File)) {
            viewContextMenu->addAction(m_menuAct->actionCopyFile);
            viewContextMenu->addAction(m_menuAct->actionProcessSha_toClipboard);
            viewContextMenu->addMenu(m_menuAct->menuAlgorithm(p_settings->algorithm()));
            viewContextMenu->addMenu(m_menuAct->menuCreateDigest);

            const QString copied = copiedDigest();
            if (!copied.isEmpty()) {
                QString str = QStringLiteral(u"Check the file by checksum: ") + format::shortenString(copied, 20);
                m_menuAct->actionCheckFileByClipboardChecksum->setText(str);
                viewContextMenu->addSeparator();
                viewContextMenu->addAction(m_menuAct->actionCheckFileByClipboardChecksum);
            }
        } else if (isMode(DbFile)) {
            viewContextMenu->addAction(m_menuAct->actionOpenDatabase);
        } else if (isMode(SumFile)) {
            viewContextMenu->addAction(m_menuAct->actionCheckSumFile);
        }
    }

    viewContextMenu->exec(p_view->viewport()->mapToGlobal(point));
}

void ModeSelector::createContextMenu_ViewDb(const QPoint &point)
{
    // TreeModel or ProxyModel View
    if (!p_view->isViewDatabase())
        return;

    const DataContainer *pData = p_view->m_data;
    const Numbers &nums = pData->m_numbers;
    const QModelIndex vInd = p_view->indexAt(point);
    const QModelIndex index = p_view->isViewModel(ModelView::ModelProxy) ? pData->m_proxy->mapToSource(vInd) : vInd;
    QMenu *viewContextMenu = m_menuAct->disposableMenu();

    if (p_proc->isStarted()) {
        if (!pData->isInCreation())
            viewContextMenu->addAction(m_menuAct->actionStop);
        viewContextMenu->addAction(m_menuAct->actionCancelBackToFS);
    } else {
        viewContextMenu->addAction(m_menuAct->actionShowDbStatus);
        viewContextMenu->addAction(m_menuAct->actionResetDb);

        // TODO: should be optimized with more clear db file state
        if (QFileInfo::exists(pData->backupFilePath())
            || (pData->isDbFileState(DbFileState::NotSaved) && QFileInfo::exists(pData->m_metadata.dbFilePath)))
        {
            viewContextMenu->addAction(m_menuAct->actionForgetChanges);
        }

        viewContextMenu->addSeparator();
        viewContextMenu->addAction(m_menuAct->actionShowFilesystem);
        if (p_view->isViewFiltered())
            viewContextMenu->addAction(m_menuAct->actionShowAll);
        viewContextMenu->addSeparator();

        // filter view
        if (p_view->isViewModel(ModelView::ModelProxy)) {
            if (nums.contains(FileStatus::Mismatched)) {
                m_menuAct->actionFilterMismatches->setChecked(p_view->isViewFiltered(FileStatus::Mismatched));
                viewContextMenu->addAction(m_menuAct->actionFilterMismatches);
            }

            if (nums.contains(FileStatus::CombUnreadable)) {
                m_menuAct->actionFilterUnreadable->setChecked(p_view->isViewFiltered(FileStatus::CombUnreadable));
                viewContextMenu->addAction(m_menuAct->actionFilterUnreadable);
            }

            if (nums.contains(FileStatus::NotCheckedMod)) {
                m_menuAct->actionFilterModified->setChecked(p_view->isViewFiltered(FileStatus::NotCheckedMod));
                viewContextMenu->addAction(m_menuAct->actionFilterModified);
            }

            if (nums.contains(FileStatus::CombNewLost)) {
                m_menuAct->actionFilterNewLost->setChecked(p_view->isViewFiltered(FileStatus::New));
                viewContextMenu->addAction(m_menuAct->actionFilterNewLost);
            }

            if (nums.contains(FileStatus::CombUpdatable))
                viewContextMenu->addSeparator();
        }

        if (!isDbConst() && pData->hasPossiblyMovedItems())
            viewContextMenu->addAction(m_menuAct->actionUpdDbFindMoved);

        if (index.isValid()) {
            if (TreeModel::isFileRow(index)) {
                // Updatable file item
                if (!isDbConst() && TreeModel::hasStatus(FileStatus::CombUpdatable, index)) {
                    switch (TreeModel::itemFileStatus(index)) {
                        case FileStatus::New:
                            viewContextMenu->addAction(m_menuAct->actionUpdFileAdd);
                            // paste from clipboard
                            if (p_settings->allowPasteIntoDb && !copiedDigest(pData->m_metadata.algorithm).isEmpty())
                                viewContextMenu->addAction(m_menuAct->actionUpdFilePasteDigest);
                            // import from digest file
                            if (QFileInfo::exists(pData->digestFilePath(index)))
                                viewContextMenu->addAction(m_menuAct->actionUpdFileImportDigest);
                            break;
                        case FileStatus::Missing:
                            viewContextMenu->addAction(m_menuAct->actionUpdFileRemove);
                            break;
                        case FileStatus::Mismatched:
                            viewContextMenu->addAction(m_menuAct->actionUpdFileReChecksum);
                            break;
                        default: break;
                    }
                }

                if (TreeModel::hasReChecksum(index))
                    viewContextMenu->addAction(m_menuAct->actionCopyReChecksum);
                else if (TreeModel::hasChecksum(index))
                    viewContextMenu->addAction(m_menuAct->actionCopyStoredChecksum);

                // Available file item
                if (TreeModel::hasStatus(FileStatus::CombAvailable, index)) {
                    viewContextMenu->addAction(m_menuAct->actionExportSum);
                    viewContextMenu->addAction(m_menuAct->actionCopyFile);

                    viewContextMenu->addAction(m_menuAct->actionCheckCurFileFromModel);
                }
            } else { // Folder item
                const bool has_avail = TreeModel::contains(FileStatus::CombAvailable, index);
                const bool has_new = TreeModel::contains(FileStatus::New, index);

                if (has_avail || has_new) {
                    viewContextMenu->addAction(m_menuAct->actionCopyFolder);

                    // contains branch db file
                    if (!p_view->m_data->branch_path_existing(index).isEmpty()) {
                        viewContextMenu->addAction(m_menuAct->actionBranchOpen);
                        if (has_new && !isDbConst())
                            viewContextMenu->addAction(m_menuAct->actionBranchImport);
                    } else {
                        viewContextMenu->addAction(m_menuAct->actionBranchMake);
                    }

                    if (has_avail)
                        viewContextMenu->addAction(m_menuAct->actionCheckItemSubfolder);
                }
            }
        }

        if (nums.contains(FileStatus::NotCheckedMod))
            viewContextMenu->addAction(m_menuAct->actionCheckAllMod);

        viewContextMenu->addAction(m_menuAct->actionCheckAll);

        if (!isDbConst() && nums.contains(FileStatus::CombUpdatable))
            viewContextMenu->addMenu(m_menuAct->menuUpdateDb(nums));
    }

    viewContextMenu->addSeparator();
    viewContextMenu->addAction(m_menuAct->actionCollapseAll);
    viewContextMenu->addAction(m_menuAct->actionExpandAll);

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
    return (p_view->m_data && p_view->m_data->isImmutable());
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
