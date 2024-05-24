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

ModeSelector::ModeSelector(View *view, QPushButton *button, Settings *settings, QObject *parent)
    : QObject{parent}, view_(view), button_(button), settings_(settings)
{
    connect(this, &ModeSelector::getPathInfo, this, &ModeSelector::cancelProcess);
    connect(this, &ModeSelector::makeFolderContentsList, this, &ModeSelector::cancelProcess);
    connect(this, &ModeSelector::makeFolderContentsFilter, this, &ModeSelector::cancelProcess);

    connect(this, &ModeSelector::updateDatabase, view_, &View::setViewSource);
    connect(this, &ModeSelector::verify, this, [=](const QModelIndex &ind){ if (!TreeModel::isFileRow(ind)) view_->setViewSource(); });

    connect(this, &ModeSelector::resetDatabase, view_, &View::saveHeaderState);

    iconProvider.setTheme(view_->palette());

    menuOpenRecent->setToolTipsVisible(true);

    actionFilterNewLost->setCheckable(true);
    actionFilterMismatches->setCheckable(true);

    actionSetAlgoSha1->setCheckable(true);
    actionSetAlgoSha256->setCheckable(true);
    actionSetAlgoSha512->setCheckable(true);

    actionGroupSelectAlgo->addAction(actionSetAlgoSha1);
    actionGroupSelectAlgo->addAction(actionSetAlgoSha256);
    actionGroupSelectAlgo->addAction(actionSetAlgoSha512);

    menuAlgo->addActions(actionGroupSelectAlgo->actions());
    menuStoreSummary->addActions(actionsMakeSummaries);

    setActionsIcons();
    connectActions();
}

void ModeSelector::connectActions()
{
    // MainWindow menu
    connect(actionShowFilesystem, &QAction::triggered, this, &ModeSelector::showFileSystem);
    connect(actionClearRecent, &QAction::triggered, settings_, &Settings::clearRecentFiles);

    // File system View
    connect(actionToHome, &QAction::triggered, view_, &View::toHome);
    connect(actionCancel, &QAction::triggered, this, &ModeSelector::cancelProcess);
    connect(actionShowFolderContentsTypes, &QAction::triggered, this, &ModeSelector::showFolderContentTypes);
    connect(actionProcessChecksumsNoFilter, &QAction::triggered, this, &ModeSelector::processChecksumsNoFilter);
    connect(actionProcessChecksumsPermFilter, &QAction::triggered, this, &ModeSelector::processChecksumsPermFilter);
    connect(actionProcessChecksumsCustomFilter, &QAction::triggered, this, &ModeSelector::processChecksumsFiltered);
    connect(actionCheckFileByClipboardChecksum, &QAction::triggered, this, &ModeSelector::checkFileByClipboardChecksum);
    connect(actionProcessSha1File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha1); });
    connect(actionProcessSha256File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha256); });
    connect(actionProcessSha512File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha512); });
    connect(actionProcessSha_toClipboard, &QAction::triggered, this,
            [=]{ emit processFileSha(view_->curPathFileSystem, settings_->algorithm(), DestFileProc::Clipboard); });
    connect(actionOpenDatabase, &QAction::triggered, this, &ModeSelector::doWork);
    connect(actionCheckSumFile , &QAction::triggered, this, &ModeSelector::doWork);

    // DB Model View
    connect(actionCancelBackToFS, &QAction::triggered, this, &ModeSelector::showFileSystem);
    connect(actionShowDbStatus, &QAction::triggered, view_, &View::showDbStatus);
    connect(actionResetDb, &QAction::triggered, this, &ModeSelector::resetDatabase);
    connect(actionForgetChanges, &QAction::triggered, this, &ModeSelector::restoreDatabase);
    connect(actionUpdateDbWithReChecksums, &QAction::triggered, this, [=]{ emit updateDatabase(DestDbUpdate::DestUpdateMismatches); });
    connect(actionUpdateDbWithNewLost, &QAction::triggered, this, [=]{ emit updateDatabase(DestDbUpdate::DestUpdateNewLost); });
    connect(actionDbAddNew, &QAction::triggered, this, [=]{ emit updateDatabase(DestDbUpdate::DestAddNew); });
    connect(actionDbClearLost, &QAction::triggered, this, [=]{ emit updateDatabase(DestDbUpdate::DestClearLost); });
    connect(actionFilterNewLost, &QAction::triggered, this,
            [=](bool isChecked){ view_->editFilter(FileStatus::FlagNewLost, isChecked); });
    connect(actionFilterMismatches, &QAction::triggered, this,
            [=](bool isChecked){ view_->editFilter(FileStatus::Mismatched, isChecked); });
    connect(actionShowAll, &QAction::triggered, view_, &View::disableFilter);
    connect(actionCheckCurFileFromModel, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(actionCheckCurSubfolderFromModel, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(actionCheckAll, &QAction::triggered, this, &ModeSelector::verifyDb);
    connect(actionCopyStoredChecksum, &QAction::triggered, this, [=]{ copyDataToClipboard(Column::ColumnChecksum); });
    connect(actionCopyReChecksum, &QAction::triggered, this, [=]{ copyDataToClipboard(Column::ColumnReChecksum); });
    connect(actionCopyItem, &QAction::triggered, this, &ModeSelector::copyItem);
    connect(actionBranchMake, &QAction::triggered, this, [=]{ emit branchSubfolder(view_->curIndexSource); });
    connect(actionBranchOpen, &QAction::triggered, this, &ModeSelector::openBranchDb);

    connect(actionCollapseAll, &QAction::triggered, view_, &View::collapseAll);
    connect(actionExpandAll, &QAction::triggered, view_, &View::expandAll);

    // Algorithm selection
    connect(actionSetAlgoSha1, &QAction::triggered, this, [=]{ settings_->setAlgorithm(QCryptographicHash::Sha1); });
    connect(actionSetAlgoSha256, &QAction::triggered, this, [=]{ settings_->setAlgorithm(QCryptographicHash::Sha256); });
    connect(actionSetAlgoSha512, &QAction::triggered, this, [=]{ settings_->setAlgorithm(QCryptographicHash::Sha512); });
}

void ModeSelector::setActionsIcons()
{
    // MainWindow menu
    actionOpenFolder->setIcon(iconProvider.icon(Icons::Folder));
    actionOpenDatabaseFile->setIcon(iconProvider.icon(Icons::Database));
    actionShowFilesystem->setIcon(iconProvider.icon(Icons::FileSystem));
    actionOpenDialogSettings->setIcon(iconProvider.icon(Icons::Configure));

    menuOpenRecent->menuAction()->setIcon(iconProvider.icon(Icons::Clock));
    actionClearRecent->setIcon(iconProvider.icon(Icons::ClearHistory));

    // File system View
    actionToHome->setIcon(iconProvider.icon(Icons::GoHome));
    actionCancel->setIcon(iconProvider.icon(Icons::Cancel));
    actionShowFolderContentsTypes->setIcon(iconProvider.icon(Icons::ChartPie));
    actionProcessChecksumsNoFilter->setIcon(iconProvider.icon(Icons::Folder));
    actionProcessChecksumsPermFilter->setIcon(iconProvider.icon(Icons::Filter));
    actionProcessChecksumsCustomFilter->setIcon(iconProvider.icon(Icons::FolderSync));
    actionCheckFileByClipboardChecksum->setIcon(iconProvider.icon(Icons::Paste));
    actionProcessSha_toClipboard->setIcon(iconProvider.icon(Icons::Copy));
    actionOpenDatabase->setIcon(iconProvider.icon(Icons::Database));
    actionCheckSumFile->setIcon(iconProvider.icon(Icons::Scan));

    menuAlgo->menuAction()->setIcon(iconProvider.icon(Icons::DoubleGear));
    menuStoreSummary->menuAction()->setIcon(iconProvider.icon(Icons::Save));

    // DB Model View
    actionCancelBackToFS->setIcon(iconProvider.icon(Icons::ProcessStop));
    actionShowDbStatus->setIcon(iconProvider.icon(Icons::Database));
    actionResetDb->setIcon(iconProvider.icon(Icons::Undo));
    actionForgetChanges->setIcon(iconProvider.icon(Icons::Backup));
    actionUpdateDbWithReChecksums->setIcon(iconProvider.icon(FileStatus::Updated));
    actionUpdateDbWithNewLost->setIcon(iconProvider.icon(Icons::Update));
    actionDbAddNew->setIcon(iconProvider.icon(FileStatus::Added));
    actionDbClearLost->setIcon(iconProvider.icon(FileStatus::Removed));
    actionFilterNewLost->setIcon(iconProvider.icon(Icons::NewFile));
    actionFilterMismatches->setIcon(iconProvider.icon(Icons::DocClose));
    actionCheckCurFileFromModel->setIcon(iconProvider.icon(Icons::Scan));
    actionCheckCurSubfolderFromModel->setIcon(iconProvider.icon(Icons::FolderSync));
    actionCheckAll->setIcon(iconProvider.icon(Icons::Start));
    actionCopyStoredChecksum->setIcon(iconProvider.icon(Icons::Copy));
    actionCopyReChecksum->setIcon(iconProvider.icon(Icons::Copy));
    actionCopyItem->setIcon(iconProvider.icon(Icons::Copy));
    actionBranchMake->setIcon(iconProvider.icon(Icons::AddFork));
    actionBranchOpen->setIcon(iconProvider.icon(Icons::Branch));
}

void ModeSelector::setProcState(ProcState *procState)
{
    proc_ = procState;
}

void ModeSelector::cancelProcess()
{
    if (proc_->isStarted())
        proc_->setState(State::Cancel);
}

void ModeSelector::abortProcess()
{
    if (proc_->isStarted())
        proc_->setState(State::Abort);
}

void ModeSelector::setMode()
{
    if (proc_->isStarted())
        return;

    if (view_->isCurrentViewModel(ModelView::NotSetted)) {
        qDebug() << "ModeSelector | Insufficient data to set mode";
        return;
    }

    if (view_->isViewFileSystem()) {
        curMode_ = selectMode(view_->curPathFileSystem);
    }
    else if (view_->isViewDatabase()) {
        curMode_ = selectMode(view_->data_->numbers);
    }
}

Mode ModeSelector::selectMode(const QString &path)
{
    QFileInfo pathInfo(path);

    if (pathInfo.isDir())
        return Folder;
    else if (pathInfo.isFile()) {
        if (tools::isDatabaseFile(path))
            return DbFile;
        else if (tools::isSummaryFile(path))
            return SumFile;
        else
            return File;
    }
    else
        return curMode_;
}

Mode ModeSelector::selectMode(const Numbers &numbers)
{
    if (numbers.contains(FileStatus::Mismatched))
        return UpdateMismatch;
    else if (numbers.contains(FileStatus::FlagNewLost))
        return ModelNewLost;
    else
        return Model;
}

void ModeSelector::getInfoPathItem()
{
    if (proc_->isState(State::StartVerbose))
        return;

    if (view_->isViewFileSystem()) {
        emit getPathInfo(view_->curPathFileSystem);
    }
    else if (view_->isViewDatabase()) {
        emit getIndexInfo(view_->curIndexSource);
    }
}

QString ModeSelector::getButtonText()
{
    if (proc_->isState(State::StartVerbose))
        return "Cancel";

    switch (curMode_) {
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
    if (proc_->isState(State::StartVerbose))
        return iconProvider.icon(Icons::Cancel);

    switch (curMode_) {
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
    if (proc_->isState(State::StartVerbose))
        return QString();

    switch (curMode_) {
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

Mode ModeSelector::currentMode()
{
    return curMode_;
}

bool ModeSelector::isCurrentMode(const Modes mode)
{
    return (mode & curMode_);
}

// tasks execution --->>>
void ModeSelector::procSumFile(QCryptographicHash::Algorithm algo)
{
    emit processFileSha(view_->curPathFileSystem, algo, DestFileProc::SumFile);
}

void ModeSelector::verifyItem()
{
    emit verify(view_->curIndexSource);
}

void ModeSelector::verifyDb()
{
    emit verify();
}

void ModeSelector::showFolderContentTypes()
{
    emit makeFolderContentsList(view_->curPathFileSystem);
}

void ModeSelector::checkFileByClipboardChecksum()
{
    emit checkFile(view_->curPathFileSystem, QGuiApplication::clipboard()->text());
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

void ModeSelector::showFileSystem()
{
    // without abort prompt
    /*if (isProcessing())
        emit cancelProcess();

    view_->setFileSystemModel();*/

    // with abort prompt
    openFsPath(QString());
}

void ModeSelector::copyDataToClipboard(Column column)
{
    if (view_->isViewDatabase()
        && view_->curIndexSource.isValid()) {

        QString strData = TreeModel::siblingAtRow(view_->curIndexSource, column).data().toString();
        if (!strData.isEmpty())
            QGuiApplication::clipboard()->setText(strData);
    }
}

void ModeSelector::openFsPath(const QString &path)
{
    if (processAbortPrompt()) {
        if (QFileInfo::exists(path))
            view_->curPathFileSystem = path;

        view_->setFileSystemModel();
    }
}

void ModeSelector::openJsonDatabase(const QString &filePath)
{
    if (processAbortPrompt())
        emit parseJsonFile(filePath);
}

void ModeSelector::openBranchDb()
{
    if (!view_->data_)
        return;

    QString assumedPath = view_->data_->getBranchFilePath(view_->curIndexSource, true);

    if (QFileInfo::exists(assumedPath))
        openJsonDatabase(assumedPath);
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
        emit makeFolderContentsFilter(view_->curPathFileSystem);
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

    emit processFolderSha(metaData);
}

bool ModeSelector::isSelectedCreateDb()
{
    // if a very large folder is selected, the file system iteration (info about folder contents process) may continue for some time,
    // so cancelation is needed before starting a new process
    cancelProcess();

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
        emit parseJsonFile(dbFilePath);

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
    if (proc_->isState(State::StartVerbose)) {
        if (view_->data_ && !view_->data_->metaData.isImported)
            showFileSystem();
        else
            cancelProcess();
        return;
    }

    switch (curMode_) {
        case Folder:
            processChecksumsFiltered();
            break;
        case File:
            emit processFileSha(view_->curPathFileSystem, settings_->algorithm());
            break;
        case DbFile:
            emit parseJsonFile(view_->curPathFileSystem);
            break;
        case SumFile:
            emit checkSummaryFile(view_->curPathFileSystem);
            break;
        case Model:
            emit verify();
            break;
        case ModelNewLost:
            emit updateDatabase(DestDbUpdate::DestUpdateNewLost);
            break;
        case UpdateMismatch:
            emit updateDatabase(DestDbUpdate::DestUpdateMismatches);
            break;
        case NoMode:
            showFileSystem();
            break;
        default:
            qDebug() << "MainWindow::doWork() | Wrong MODE:" << curMode_;
            break;
    }
}

void ModeSelector::quickAction()
{
    if (proc_->isStarted())
            return;

    switch (curMode_) {
        case File:
            doWork();
            break;
        case DbFile:
            emit parseJsonFile(view_->curPathFileSystem);
            break;
        case SumFile:
            emit checkSummaryFile(view_->curPathFileSystem);
            break;
        case Model:
            if (TreeModel::isFileRow(view_->curIndexSource))
                emit verify(view_->curIndexSource);
            break;
        case ModelNewLost:
            if (TreeModel::isFileRow(view_->curIndexSource))
                emit verify(view_->curIndexSource);
            break;
        case UpdateMismatch:
            if (TreeModel::isFileRow(view_->curIndexSource))
                emit verify(view_->curIndexSource);
            break;
        default: break;
    }
}

void ModeSelector::createContextMenu_View(const QPoint &point)
{
    QModelIndex index = view_->indexAt(point);
    QMenu *viewContextMenu = new QMenu(view_);
    connect(viewContextMenu, &QMenu::aboutToHide, viewContextMenu, &QMenu::deleteLater);

    if (view_->isCurrentViewModel(ModelView::NotSetted)) {
        viewContextMenu->addAction(actionShowFilesystem);
    }
    // Filesystem View
    else if (view_->isViewFileSystem()) {
        viewContextMenu->addAction(actionToHome);
        viewContextMenu->addSeparator();

        if (proc_->isState(State::StartVerbose))
            viewContextMenu->addAction(actionCancel);
        else if (index.isValid()) {
            if (isCurrentMode(Folder)) {
                viewContextMenu->addAction(actionShowFolderContentsTypes);
                viewContextMenu->addMenu(menuAlgorithm());
                viewContextMenu->addSeparator();

                viewContextMenu->addAction(actionProcessChecksumsNoFilter);
                if (settings_->filter.isFilterEnabled())
                    viewContextMenu->addAction(actionProcessChecksumsPermFilter);
                viewContextMenu->addAction(actionProcessChecksumsCustomFilter);
            }
            else if (isCurrentMode(File)) {
                viewContextMenu->addMenu(menuAlgorithm());
                viewContextMenu->addAction(actionProcessSha_toClipboard);
                viewContextMenu->addMenu(menuStoreSummary);

                QString clipboardText = QGuiApplication::clipboard()->text();
                if (tools::canBeChecksum(clipboardText)) {
                    actionCheckFileByClipboardChecksum->setText("Check the file by checksum: " + format::shortenString(clipboardText, 20));
                    viewContextMenu->addSeparator();
                    viewContextMenu->addAction(actionCheckFileByClipboardChecksum);
                }
            }
            else if (isCurrentMode(DbFile))
                viewContextMenu->addAction(actionOpenDatabase);
            else if (isCurrentMode(SumFile))
                viewContextMenu->addAction(actionCheckSumFile);
        }
    }
    // TreeModel or ProxyModel View
    else if (view_->isViewDatabase()) {
        if (proc_->isStarted()) {
            if (view_->data_->metaData.isImported)
                viewContextMenu->addAction(actionCancel);
            viewContextMenu->addAction(actionCancelBackToFS);
        }
        else {
            viewContextMenu->addAction(actionShowDbStatus);
            viewContextMenu->addAction(actionResetDb);
            if (QFile::exists(view_->data_->backupFilePath()))
                viewContextMenu->addAction(actionForgetChanges);

            viewContextMenu->addSeparator();
            viewContextMenu->addAction(actionShowFilesystem);
            if (view_->isViewFiltered())
                viewContextMenu->addAction(actionShowAll);
            viewContextMenu->addSeparator();

            if (view_->isCurrentViewModel(ModelView::ModelProxy)) {
                if (view_->data_->numbers.contains(FileStatus::Mismatched)) {
                    actionFilterMismatches->setChecked(view_->isViewFiltered(FileStatus::Mismatched));
                    viewContextMenu->addAction(actionFilterMismatches);
                }

                if (view_->data_->numbers.contains(FileStatus::FlagNewLost)) {
                    actionFilterNewLost->setChecked(view_->isViewFiltered(FileStatus::New));
                    viewContextMenu->addAction(actionFilterNewLost);
                }

                if (view_->data_->numbers.contains(FileStatus::FlagUpdatable))
                    viewContextMenu->addSeparator();
            }

            if (index.isValid()) {
                 if (TreeModel::isFileRow(index)) {
                    if (TreeModel::hasReChecksum(index))
                        viewContextMenu->addAction(actionCopyReChecksum);
                    else if (TreeModel::hasChecksum(index))
                        viewContextMenu->addAction(actionCopyStoredChecksum);

                    if (TreeModel::hasStatus(FileStatus::FlagAvailable, index)) {
                        actionCopyItem->setText("Copy File");
                        viewContextMenu->addAction(actionCopyItem);

                        viewContextMenu->addAction(actionCheckCurFileFromModel);
                    }
                }
                else if (TreeModel::contains(FileStatus::FlagAvailable, index)) {
                    actionCopyItem->setText("Copy Folder");
                    viewContextMenu->addAction(actionCopyItem);

                    if (!view_->data_->getBranchFilePath(index, true).isEmpty())
                        viewContextMenu->addAction(actionBranchOpen);
                    else
                        viewContextMenu->addAction(actionBranchMake);

                    viewContextMenu->addAction(actionCheckCurSubfolderFromModel);
                }
            }

            viewContextMenu->addAction(actionCheckAll);

            if (view_->data_->contains(FileStatus::FlagUpdatable))
                viewContextMenu->addMenu(menuUpdateDb());
        }

        viewContextMenu->addSeparator();
        viewContextMenu->addAction(actionCollapseAll);
        viewContextMenu->addAction(actionExpandAll);
    }

    viewContextMenu->exec(view_->viewport()->mapToGlobal(point));
}

void ModeSelector::updateMenuOpenRecent()
{
    menuOpenRecent->clear();
    menuOpenRecent->setDisabled(settings_->recentFiles.isEmpty());

    if (!menuOpenRecent->isEnabled())
        return;

    QIcon dbIcon = iconProvider.icon(Icons::Database);

    foreach (const QString &recentFilePath, settings_->recentFiles) {
        if (QFileInfo::exists(recentFilePath)) {
            QAction *act = menuOpenRecent->addAction(dbIcon, paths::basicName(recentFilePath));
            act->setToolTip(recentFilePath);
            connect(act, &QAction::triggered, this, [=]{openJsonDatabase(recentFilePath);});
        }
    }

    menuOpenRecent->addSeparator();
    menuOpenRecent->addAction(actionClearRecent);
}

QMenu* ModeSelector::menuAlgorithm()
{
    switch (settings_->algorithm()) {
    case QCryptographicHash::Sha1:
        actionSetAlgoSha1->setChecked(true);
        break;
    case QCryptographicHash::Sha256:
        actionSetAlgoSha256->setChecked(true);
        break;
    case QCryptographicHash::Sha512:
        actionSetAlgoSha512->setChecked(true);
        break;
    default:
        actionSetAlgoSha256->setChecked(true);
        break;
    }

    menuAlgo->menuAction()->setText("Algorithm " + format::algoToStr(settings_->algorithm()));

    return menuAlgo;
}

QMenu* ModeSelector::menuUpdateDb()
{
    if (!view_->data_)
        return menuUpdateDatabase;

    if (!menuUpdateDatabase) {
        menuUpdateDatabase = new QMenu("Update the Database", view_);
        menuUpdateDatabase->menuAction()->setIcon(iconProvider.icon(Icons::Update));

        menuUpdateDatabase->addAction(actionDbAddNew);
        menuUpdateDatabase->addAction(actionDbClearLost);
        menuUpdateDatabase->addAction(actionUpdateDbWithNewLost);
        menuUpdateDatabase->addSeparator();
        menuUpdateDatabase->addAction(actionUpdateDbWithReChecksums);
    }

    actionDbAddNew->setEnabled(view_->data_->contains(FileStatus::New));
    actionDbClearLost->setEnabled(view_->data_->contains(FileStatus::Missing));
    actionUpdateDbWithNewLost->setEnabled(actionDbAddNew->isEnabled() || actionDbClearLost->isEnabled());
    actionUpdateDbWithReChecksums->setEnabled(view_->data_->contains(FileStatus::Mismatched));

    return menuUpdateDatabase;
}

void ModeSelector::createContextMenu_Button(const QPoint &point)
{
    if (!proc_->isStarted() && isCurrentMode(File | Folder))
        menuAlgorithm()->exec(button_->mapToGlobal(point));
}

bool ModeSelector::processAbortPrompt()
{
    if (!proc_->isState(State::StartVerbose))
        return true;

    QMessageBox msgBox(view_);
    msgBox.setWindowTitle("Processing...");
    msgBox.setText("Abort current process?");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    int ret = msgBox.exec();

    if (ret == QMessageBox::Yes) {
        abortProcess();
        return true;
    }
    else
        return false;
}
