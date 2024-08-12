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

ModeSelector::ModeSelector(View *view, Settings *settings, QObject *parent)
    : QObject{parent}, view_(view), settings_(settings)
{
    iconProvider.setTheme(view_->palette());
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
    connect(menuAct_->actionProcessChecksumsPermFilter, &QAction::triggered, this, &ModeSelector::processChecksumsPermFilter);
    connect(menuAct_->actionProcessChecksumsCustomFilter, &QAction::triggered, this, &ModeSelector::processChecksumsFiltered);
    connect(menuAct_->actionCheckFileByClipboardChecksum, &QAction::triggered, this, &ModeSelector::checkFileByClipboardChecksum);
    connect(menuAct_->actionProcessSha1File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha1); });
    connect(menuAct_->actionProcessSha256File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha256); });
    connect(menuAct_->actionProcessSha512File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha512); });
    connect(menuAct_->actionProcessSha_toClipboard, &QAction::triggered, this,
            [=]{ processFileSha(view_->curPathFileSystem, settings_->algorithm(), DestFileProc::Clipboard); });
    connect(menuAct_->actionOpenDatabase, &QAction::triggered, this, &ModeSelector::doWork);
    connect(menuAct_->actionCheckSumFile , &QAction::triggered, this, &ModeSelector::doWork);

    // DB Model View
    connect(menuAct_->actionCancelBackToFS, SIGNAL(triggered()), this, SLOT(showFileSystem()));
    connect(menuAct_->actionShowDbStatus, &QAction::triggered, view_, &View::showDbStatus);
    connect(menuAct_->actionResetDb, &QAction::triggered, this, &ModeSelector::resetDatabase);
    connect(menuAct_->actionForgetChanges, &QAction::triggered, this, &ModeSelector::restoreDatabase);
    connect(menuAct_->actionUpdateDbWithReChecksums, &QAction::triggered, this, [=]{ updateDatabase(DestDbUpdate::DestUpdateMismatches); });
    connect(menuAct_->actionUpdateDbWithNewLost, &QAction::triggered, this, [=]{ updateDatabase(DestDbUpdate::DestUpdateNewLost); });
    connect(menuAct_->actionDbAddNew, &QAction::triggered, this, [=]{ updateDatabase(DestDbUpdate::DestAddNew); });
    connect(menuAct_->actionDbClearLost, &QAction::triggered, this, [=]{ updateDatabase(DestDbUpdate::DestClearLost); });
    connect(menuAct_->actionFilterNewLost, &QAction::triggered, this,
            [=](bool isChecked){ view_->editFilter(FileStatus::FlagNewLost, isChecked); });
    connect(menuAct_->actionFilterMismatches, &QAction::triggered, this,
            [=](bool isChecked){ view_->editFilter(FileStatus::Mismatched, isChecked); });
    connect(menuAct_->actionFilterUnreadable, &QAction::triggered, this,
            [=](bool isChecked){ view_->editFilter(FileStatus::Unreadable, isChecked); });
    connect(menuAct_->actionShowAll, &QAction::triggered, view_, &View::disableFilter);
    connect(menuAct_->actionCheckCurFileFromModel, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(menuAct_->actionCheckCurSubfolderFromModel, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(menuAct_->actionCheckAll, &QAction::triggered, this, &ModeSelector::verifyDb);
    connect(menuAct_->actionCopyStoredChecksum, &QAction::triggered, this, [=]{ copyDataToClipboard(Column::ColumnChecksum); });
    connect(menuAct_->actionCopyReChecksum, &QAction::triggered, this, [=]{ copyDataToClipboard(Column::ColumnReChecksum); });
    connect(menuAct_->actionCopyItem, &QAction::triggered, this, &ModeSelector::copyItem);
    connect(menuAct_->actionBranchMake, &QAction::triggered, this, [=]{ branchSubfolder(view_->curIndexSource); });
    connect(menuAct_->actionBranchOpen, &QAction::triggered, this, &ModeSelector::openBranchDb);
    connect(menuAct_->actionUpdFileAdd, &QAction::triggered, this, &ModeSelector::updateDbItem);
    connect(menuAct_->actionUpdFileRemove, &QAction::triggered, this, &ModeSelector::updateDbItem);
    connect(menuAct_->actionUpdFileReChecksum, &QAction::triggered, this, &ModeSelector::updateDbItem);

    connect(menuAct_->actionCollapseAll, &QAction::triggered, view_, &View::collapseAll);
    connect(menuAct_->actionExpandAll, &QAction::triggered, view_, &View::expandAll);

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

    abortProcess();
    if (view_->isViewFileSystem()) {
        manager_->queueTask(&Manager::getPathInfo, view_->curPathFileSystem);
    }
    else if (view_->isViewDatabase()) {
        manager_->queueTask(&Manager::getIndexInfo, view_->curIndexSource); // info about database item (file or subfolder index)
    }
}

QString ModeSelector::getButtonText()
{
    switch (mode()) {
        case FileProcessing:
        case DbProcessing:
            return "Stop";
        case DbCreating:
            return "Abort";
        case Folder:
        case File:
            return format::algoToStr(settings_->algorithm());
        case DbFile:
            return "Open";
        case SumFile:
            return "Check";
        case Model:
            return "Verify";
        case ModelNewLost:
            return "New/Lost";
        case UpdateMismatch:
            return "Update";
        case NoMode:
            return "Browse";
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
            return iconProvider.icon(Icons::ProcessStop);
        case DbCreating:
            return iconProvider.icon(Icons::ProcessAbort);
        case Folder:
            return iconProvider.icon(Icons::FolderSync);
        case File:
            return iconProvider.icon(Icons::HashFile);
        case DbFile:
            return iconProvider.icon(Icons::Database);
        case SumFile:
            return iconProvider.icon(Icons::Scan);
        case Model:
            return iconProvider.icon(Icons::Start);
        case ModelNewLost:
            return iconProvider.icon(Icons::Update);
        case UpdateMismatch:
            return iconProvider.icon(FileStatus::Updated);
        default:
            break;
    }

    return QIcon();
}

QString ModeSelector::getButtonToolTip()
{
    switch (mode()) {
        case Folder:
            return "Calculate checksums of contained files\nand save the result to the local database";
        case File:
            return QString("Calculate %1 checksum of the file").arg(format::algoToStr(settings_->algorithm()));
        case Model:
            return "Check ALL files against stored checksums";
        case ModelNewLost:
            return "Update the Database:\nadd new files, delete missing ones";
        case UpdateMismatch:
            return "Update mismatched checksums with newly calculated ones";
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

        if (view_->data_->numbers.contains(FileStatus::Mismatched))
            return UpdateMismatch;
        else if (view_->data_->numbers.contains(FileStatus::FlagNewLost))
            return ModelNewLost;
        else
            return Model;
    }

    if (view_->isViewFileSystem()) {
        if (proc_ && proc_->isState(State::StartVerbose))
            return FileProcessing;

        QFileInfo pathInfo(view_->curPathFileSystem);
        if (pathInfo.isDir())
            return Folder;
        else if (pathInfo.isFile()) {
            if (tools::isDatabaseFile(view_->curPathFileSystem))
                return DbFile;
            else if (tools::isSummaryFile(view_->curPathFileSystem))
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
    processFileSha(view_->curPathFileSystem, algo, DestFileProc::SumFile);
}

void ModeSelector::promptItemFileUpd()
{
    FileStatus storedStatus = TreeModel::itemFileStatus(view_->curIndexSource);

    if (!(storedStatus & FileStatus::FlagUpdatable))
        return;

    QMessageBox msgBox(view_);
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);

    switch (storedStatus) {
    case FileStatus::New:
        static const QPixmap icoNew = iconProvider.icon(FileStatus::New).pixmap(64, 64);
        msgBox.setIconPixmap(icoNew);
        msgBox.setWindowTitle("New File...");
        msgBox.setText("The database does not yet contain\n"
                       "a corresponding checksum.");
        msgBox.setInformativeText("Would you like to calculate and add it?");
        msgBox.button(QMessageBox::Ok)->setText("Add");
        msgBox.button(QMessageBox::Ok)->setIcon(iconProvider.icon(FileStatus::Added));
        break;
    case FileStatus::Missing:
        static const QPixmap icoMissing = iconProvider.icon(FileStatus::Missing).pixmap(64, 64);
        msgBox.setIconPixmap(icoMissing);
        msgBox.setWindowTitle("Missing File...");
        msgBox.setText("File does not exist.");
        msgBox.setInformativeText("Remove the Item from the database?");
        msgBox.button(QMessageBox::Ok)->setText("Remove");
        msgBox.button(QMessageBox::Ok)->setIcon(iconProvider.icon(FileStatus::Removed));
        break;
    case FileStatus::Mismatched:
        static const QPixmap icoMismatch = iconProvider.icon(FileStatus::Mismatched).pixmap(64, 64);
        msgBox.setIconPixmap(icoMismatch);
        msgBox.setWindowTitle("Mismatched Checksum...");
        msgBox.setText("The calculated and stored checksums do not match.");
        msgBox.setInformativeText("Do you want to update the stored one?");
        msgBox.button(QMessageBox::Ok)->setText("Update");
        msgBox.button(QMessageBox::Ok)->setIcon(iconProvider.icon(FileStatus::Updated));
        break;
    default:
        break;
    }

    if (msgBox.exec() == QMessageBox::Ok)
        updateDbItem();
}

void ModeSelector::verifyItem()
{
    verify(view_->curIndexSource);
}

void ModeSelector::verifyDb()
{
    verify();
}

void ModeSelector::updateDbItem()
{
    /*  At the moment, updating a separate file
     *  with the View filtering enabled is unstable,
     *  so the filter should be disabled before making changes.
     */
    if (view_->isViewFiltered())
        view_->disableFilter();

    manager_->queueTask(&Manager::updateItemFile, view_->curIndexSource);
}

void ModeSelector::showFolderContentTypes()
{
    makeFolderContentsList(view_->curPathFileSystem);
}

void ModeSelector::checkFileByClipboardChecksum()
{
    checkFile(view_->curPathFileSystem, QGuiApplication::clipboard()->text());
}

void ModeSelector::copyItem()
{
    if (!view_->isViewDatabase() || !view_->curIndexSource.isValid())
        return;

    QString itemPath = view_->data_->itemAbsolutePath(view_->curIndexSource);
    QMimeData* mimeData = new QMimeData();
    mimeData->setUrls({QUrl::fromLocalFile(itemPath)});
    QGuiApplication::clipboard()->setMimeData(mimeData);
}

void ModeSelector::showFileSystem(const QString &path)
{
    if (promptProcessAbort()) {
        if (QFileInfo::exists(path))
            view_->curPathFileSystem = path;

        if (view_->data_
            && view_->data_->isDbFileState(DbFileState::NotSaved))
        {
            manager_->queueTask(&Manager::prepareSwitchToFs);
        }
        else {
            view_->setFileSystemModel();
        }
    }
}

void ModeSelector::saveData()
{
    manager_->queueTask(&Manager::saveData);
}

void ModeSelector::copyDataToClipboard(Column column)
{
    if (view_->isViewDatabase()
        && view_->curIndexSource.isValid())
    {
        QString strData = TreeModel::siblingAtRow(view_->curIndexSource, column).data().toString();
        if (!strData.isEmpty())
            QGuiApplication::clipboard()->setText(strData);
    }
}

void ModeSelector::updateDatabase(const DestDbUpdate task)
{
    view_->setViewSource();
    manager_->queueTask(&Manager::updateDatabase, task);
}

void ModeSelector::resetDatabase()
{
    if (view_->data_) {
        view_->saveHeaderState();
        openJsonDatabase(view_->data_->metaData.databaseFilePath);
    }
}

void ModeSelector::restoreDatabase()
{
    manager_->queueTask(&Manager::restoreDatabase);
}

void ModeSelector::openJsonDatabase(const QString &filePath)
{
    if (promptProcessAbort()) {
        manager_->queueTask(&Manager::saveData);
        manager_->queueTask(&Manager::createDataModel, filePath);
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

    QString assumedPath = view_->data_->getBranchFilePath(view_->curIndexSource, true);

    if (QFileInfo::exists(assumedPath))
        openJsonDatabase(assumedPath);
}

void ModeSelector::processFileSha(const QString &path, QCryptographicHash::Algorithm algo, DestFileProc result)
{
    manager_->queueTask(&Manager::processFileSha, path, algo, result);
}

void ModeSelector::checkSummaryFile(const QString &path)
{
    manager_->queueTask(&Manager::checkSummaryFile, path);
}

void ModeSelector::checkFile(const QString &filePath, const QString &checkSum)
{
    manager_->queueTask(qOverload<const QString&, const QString&>(&Manager::checkFile), filePath, checkSum);
}

void ModeSelector::verify(const QModelIndex& index)
{
    if (TreeModel::isFileRow(index)) {
        manager_->queueTask(&Manager::verifyFileItem, index);
    }
    else {
        view_->setViewSource();
        manager_->queueTask(&Manager::verifyFolderItem, index);
    }
}

void ModeSelector::branchSubfolder(const QModelIndex &subfolder)
{
    manager_->queueTask(&Manager::branchSubfolder, subfolder);
}

void ModeSelector::makeFolderContentsList(const QString &folderPath)
{
    abortProcess();
    manager_->queueTask(&Manager::folderContentsList, folderPath, false);
}

void ModeSelector::makeFolderContentsFilter(const QString &folderPath)
{
    abortProcess();
    manager_->queueTask(&Manager::folderContentsList, folderPath, true);
}

void ModeSelector::_makeDbContentsList()
{
    if (!proc_->isStarted() && view_->isViewDatabase()) {
        manager_->queueTask(&Manager::makeDbContentsList);
    }
}

QString ModeSelector::composeDbFilePath()
{
    QString folderName = settings_->addWorkDirToFilename ? paths::basicName(view_->curPathFileSystem) : QString();
    QString databaseFileName = format::composeDbFileName(settings_->dbPrefix, folderName, settings_->dbFileExtension());

    return paths::joinPath(view_->curPathFileSystem, databaseFileName);
}

void ModeSelector::processChecksumsFiltered()
{
    if (isSelectedCreateDb())
        makeFolderContentsFilter(view_->curPathFileSystem);
}

void ModeSelector::processChecksumsNoFilter()
{
    if (isSelectedCreateDb())
        processFolderChecksums(FilterRule());
}

void ModeSelector::processChecksumsPermFilter()
{
    if (isSelectedCreateDb())
        processFolderChecksums(settings_->filter);
}

void ModeSelector::processFolderChecksums(const FilterRule &filter)
{
    MetaData metaData;
    metaData.workDir = view_->curPathFileSystem;
    metaData.algorithm = settings_->algorithm();
    metaData.filter = filter;
    metaData.databaseFilePath = composeDbFilePath();

    manager_->queueTask(&Manager::processFolderSha, metaData);
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
    msgBox.setText(QString("The folder already contains the database file:\n%1").arg(paths::basicName(dbFilePath)));
    msgBox.setInformativeText("Do you want to open or overwrite it?");
    msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::Save | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Open);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.button(QMessageBox::Save)->setText("Overwrite");
    int ret = msgBox.exec();

    if (ret == QMessageBox::Open)
        openJsonDatabase(dbFilePath);

    return (ret == QMessageBox::Save);
}

bool ModeSelector::emptyFolderPrompt()
{
    if (Files::isEmptyFolder(view_->curPathFileSystem)) {
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
            processFileSha(view_->curPathFileSystem, settings_->algorithm());
            break;
        case DbFile:
            openJsonDatabase(view_->curPathFileSystem);
            break;
        case SumFile:
            checkSummaryFile(view_->curPathFileSystem);
            break;
        case Model:
            if (!proc_->isStarted())
                verify();
            break;
        case ModelNewLost:
            if (!proc_->isStarted())
                updateDatabase(DestDbUpdate::DestUpdateNewLost);
            break;
        case UpdateMismatch:
            if (!proc_->isStarted())
                updateDatabase(DestDbUpdate::DestUpdateMismatches);
            break;
        case NoMode:
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
            openJsonDatabase(view_->curPathFileSystem);
            break;
        case SumFile:
            checkSummaryFile(view_->curPathFileSystem);
            break;
        case Model:
        case ModelNewLost:
        case UpdateMismatch:
            if (TreeModel::isFileRow(view_->curIndexSource)) {
                if (TreeModel::hasStatus(FileStatus::FlagUpdatable, view_->curIndexSource))
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
    else if (view_->isCurrentViewModel(ModelView::NotSetted))
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
            viewContextMenu->addMenu(menuAct_->menuAlgorithm(settings_->algorithm()));
            viewContextMenu->addSeparator();

            viewContextMenu->addAction(menuAct_->actionProcessChecksumsNoFilter);
            if (settings_->filter.isFilterEnabled())
                viewContextMenu->addAction(menuAct_->actionProcessChecksumsPermFilter);
            viewContextMenu->addAction(menuAct_->actionProcessChecksumsCustomFilter);
        }
        else if (isMode(File)) {
            viewContextMenu->addMenu(menuAct_->menuAlgorithm(settings_->algorithm()));
            viewContextMenu->addAction(menuAct_->actionProcessSha_toClipboard);
            viewContextMenu->addMenu(menuAct_->menuStoreSummary);

            QString clipboardText = QGuiApplication::clipboard()->text();
            if (tools::canBeChecksum(clipboardText)) {
                menuAct_->actionCheckFileByClipboardChecksum->setText("Check the file by checksum: " + format::shortenString(clipboardText, 20));
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

    QModelIndex index = view_->indexAt(point);
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

        if (view_->isCurrentViewModel(ModelView::ModelProxy)) {
            if (view_->data_->numbers.contains(FileStatus::Mismatched)) {
                menuAct_->actionFilterMismatches->setChecked(view_->isViewFiltered(FileStatus::Mismatched));
                viewContextMenu->addAction(menuAct_->actionFilterMismatches);
            }

            if (view_->data_->numbers.contains(FileStatus::Unreadable)) {
                menuAct_->actionFilterUnreadable->setChecked(view_->isViewFiltered(FileStatus::Unreadable));
                viewContextMenu->addAction(menuAct_->actionFilterUnreadable);
            }

            if (view_->data_->numbers.contains(FileStatus::FlagNewLost)) {
                menuAct_->actionFilterNewLost->setChecked(view_->isViewFiltered(FileStatus::New));
                viewContextMenu->addAction(menuAct_->actionFilterNewLost);
            }

            if (view_->data_->numbers.contains(FileStatus::FlagUpdatable))
                viewContextMenu->addSeparator();
        }

        if (index.isValid()) {
            if (TreeModel::isFileRow(index)) {
                // Updatable file item
                if (TreeModel::hasStatus(FileStatus::FlagUpdatable, index)) {
                    FileStatus status = TreeModel::itemFileStatus(index);
                    if (status == FileStatus::New)
                        viewContextMenu->addAction(menuAct_->actionUpdFileAdd);
                    else if (status == FileStatus::Missing)
                        viewContextMenu->addAction(menuAct_->actionUpdFileRemove);
                    else if (status == FileStatus::Mismatched)
                        viewContextMenu->addAction(menuAct_->actionUpdFileReChecksum);
                }

                if (TreeModel::hasReChecksum(index))
                    viewContextMenu->addAction(menuAct_->actionCopyReChecksum);
                else if (TreeModel::hasChecksum(index))
                    viewContextMenu->addAction(menuAct_->actionCopyStoredChecksum);

                // Available file item
                if (TreeModel::hasStatus(FileStatus::FlagAvailable, index)) {
                    menuAct_->actionCopyItem->setText("Copy File");
                    viewContextMenu->addAction(menuAct_->actionCopyItem);

                    viewContextMenu->addAction(menuAct_->actionCheckCurFileFromModel);
                }
            }
            // Folder item
            else if (TreeModel::contains(FileStatus::FlagAvailable, index)) {
                menuAct_->actionCopyItem->setText("Copy Folder");
                viewContextMenu->addAction(menuAct_->actionCopyItem);

                if (!view_->data_->getBranchFilePath(index, true).isEmpty())
                    viewContextMenu->addAction(menuAct_->actionBranchOpen);
                else
                    viewContextMenu->addAction(menuAct_->actionBranchMake);

                viewContextMenu->addAction(menuAct_->actionCheckCurSubfolderFromModel);
            }
        }

        viewContextMenu->addAction(menuAct_->actionCheckAll);

        if (view_->data_->contains(FileStatus::FlagUpdatable))
            viewContextMenu->addMenu(menuAct_->menuUpdateDb(view_->data_->numbers));
    }

    viewContextMenu->addSeparator();
    viewContextMenu->addAction(menuAct_->actionCollapseAll);
    viewContextMenu->addAction(menuAct_->actionExpandAll);

    viewContextMenu->exec(view_->viewport()->mapToGlobal(point));
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

    QString strAct = abort ? "Abort" : "Stop";
    QIcon icoAct = abort ? iconProvider.icon(Icons::ProcessAbort) : iconProvider.icon(Icons::ProcessStop);
    static const QPixmap icoMsgBox = iconProvider.icon(FileStatus::Calculating).pixmap(64, 64);

    QMessageBox msgBox(view_);
    connect(proc_, &ProcState::progressFinished, &msgBox, &QMessageBox::reject);

    msgBox.setIconPixmap(icoMsgBox);
    msgBox.setWindowTitle("Processing...");
    msgBox.setText(QString("%1 current process?").arg(strAct));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText(strAct);
    msgBox.button(QMessageBox::No)->setText("Continue...");
    msgBox.button(QMessageBox::Yes)->setIcon(icoAct);
    msgBox.button(QMessageBox::No)->setIcon(iconProvider.icon(Icons::DoubleGear));

    if (msgBox.exec() == QMessageBox::Yes) {
        abort ? abortProcess() : stopProcess();
        return true;
    }
    else {
        return false;
    }
}
