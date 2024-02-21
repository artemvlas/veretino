/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "modeselector.h"
#include <QFileInfo>
#include <QGuiApplication>
#include <QMessageBox>
#include <QClipboard>
#include <QDebug>

ModeSelector::ModeSelector(View *view, QPushButton *button, Settings *settings, QObject *parent)
    : QObject{parent}, view_(view), button_(button), settings_(settings)
{
    connect(this, &ModeSelector::getPathInfo, this, &ModeSelector::cancelProcess);
    connect(this, &ModeSelector::makeFolderContentsList, this, &ModeSelector::cancelProcess);
    connect(this, &ModeSelector::makeFolderContentsFilter, this, &ModeSelector::cancelProcess);

    connect(this, &ModeSelector::updateNewLost, this, &ModeSelector::prepareView);
    connect(this, &ModeSelector::updateMismatch, this, &ModeSelector::prepareView);
    connect(this, &ModeSelector::verify, this, [=](const QModelIndex &ind){if (!TreeModel::isFileRow(ind)) prepareView();});

    connect(this, &ModeSelector::resetDatabase, view_, &View::saveHeaderState);

    menuOpenRecent->setToolTipsVisible(true);

    actionShowNewLostOnly->setCheckable(true);
    actionShowMismatchesOnly->setCheckable(true);

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
    connect(actionClearRecent, &QAction::triggered, this, [=]{settings_->recentFiles.clear();});

    // File system View
    connect(actionToHome, &QAction::triggered, view_, &View::toHome);
    connect(actionCancel, &QAction::triggered, this, &ModeSelector::cancelProcess);
    connect(actionShowFolderContentsTypes, &QAction::triggered, this, &ModeSelector::showFolderContentTypes);
    connect(actionProcessContainedChecksums, &QAction::triggered, this, &ModeSelector::doWork);
    connect(actionProcessFilteredChecksums, &QAction::triggered, this, &ModeSelector::processFolderFilteredChecksums);
    connect(actionCheckFileByClipboardChecksum, &QAction::triggered, this, [=]{checkFileChecksum(QGuiApplication::clipboard()->text());});
    connect(actionProcessSha1File, &QAction::triggered, this, [=]{computeFileChecksum(QCryptographicHash::Sha1);});
    connect(actionProcessSha256File, &QAction::triggered, this, [=]{computeFileChecksum(QCryptographicHash::Sha256);});
    connect(actionProcessSha512File, &QAction::triggered, this, [=]{computeFileChecksum(QCryptographicHash::Sha512);});
    connect(actionProcessSha_toClipboard, &QAction::triggered, this, &ModeSelector::quickAction);
    connect(actionOpenDatabase, &QAction::triggered, this, &ModeSelector::doWork);
    connect(actionCheckSumFile , &QAction::triggered, this, &ModeSelector::doWork);

    // DB Model View
    connect(actionCancelBackToFS, &QAction::triggered, this, &ModeSelector::showFileSystem);
    connect(actionShowDbStatus, &QAction::triggered, view_, &View::showDbStatus);
    connect(actionResetDb, &QAction::triggered, this, &ModeSelector::resetDatabase);
    connect(actionForgetChanges, &QAction::triggered, this, &ModeSelector::restoreDatabase);
    connect(actionUpdateDbWithReChecksums, &QAction::triggered, this, &ModeSelector::updateMismatch);
    connect(actionUpdateDbWithNewLost, &QAction::triggered, this, &ModeSelector::updateNewLost);
    connect(actionShowNewLostOnly, &QAction::toggled, this, [=](bool isChecked){if (isChecked) view_->setFilter({FileStatus::New, FileStatus::Missing}); else view_->disableFilter();});
    connect(actionShowMismatchesOnly, &QAction::toggled, this, [=](bool isChecked){if (isChecked) view_->setFilter(FileStatus::Mismatched); else view_->disableFilter();});
    connect(actionShowAll, &QAction::triggered, view_, &View::disableFilter);
    connect(actionCheckCurFileFromModel, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(actionCheckCurSubfolderFromModel, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(actionCheckAll, &QAction::triggered, this, &ModeSelector::verifyDb);
    connect(actionCopyStoredChecksum, &QAction::triggered, this, [=]{copyDataToClipboard(Column::ColumnChecksum);});
    connect(actionCopyReChecksum, &QAction::triggered, this, [=]{copyDataToClipboard(Column::ColumnReChecksum);});
    connect(actionBranchSubfolder, &QAction::triggered, this, [=]{emit branchSubfolder(view_->curIndexSource);});

    connect(actionCollapseAll, &QAction::triggered, view_, &View::collapseAll);
    connect(actionExpandAll, &QAction::triggered, view_, &View::expandAll);

    // Algorithm selection
    connect(actionSetAlgoSha1, &QAction::triggered, this, [=]{setAlgorithm(QCryptographicHash::Sha1);});
    connect(actionSetAlgoSha256, &QAction::triggered, this, [=]{setAlgorithm(QCryptographicHash::Sha256);});
    connect(actionSetAlgoSha512, &QAction::triggered, this, [=]{setAlgorithm(QCryptographicHash::Sha512);});
}

void ModeSelector::setActionsIcons()
{
    QString theme = tools::themeFolder(view_->palette());

    // MainWindow menu
    actionOpenFolder->setIcon(QIcon(QString(":/icons/%1/folder.svg").arg(theme)));
    actionOpenDatabaseFile->setIcon(QIcon(QString(":/icons/%1/database.svg").arg(theme)));
    actionShowFilesystem->setIcon(QIcon(QString(":/icons/%1/filesystem.svg").arg(theme)));
    actionOpenSettingsDialog->setIcon(QIcon(QString(":/icons/%1/configure.svg").arg(theme)));

    menuOpenRecent->menuAction()->setIcon(QIcon(QString(":/icons/%1/clock.svg").arg(theme)));
    actionClearRecent->setIcon(QIcon(QString(":/icons/%1/clear-history.svg").arg(theme)));

    // File system View
    actionToHome->setIcon(QIcon(QString(":/icons/%1/go-home.svg").arg(theme)));
    actionCancel->setIcon(QIcon(QString(":/icons/%1/cancel.svg").arg(theme)));
    actionShowFolderContentsTypes->setIcon(QIcon(QString(":/icons/%1/chart-pie.svg").arg(theme)));
    actionProcessContainedChecksums->setIcon(QIcon(QString(":/icons/%1/folder-sync.svg").arg(theme)));
    actionProcessFilteredChecksums->setIcon(QIcon(QString(":/icons/%1/filter.svg").arg(theme)));
    actionCheckFileByClipboardChecksum->setIcon(QIcon(QString(":/icons/%1/paste.svg").arg(theme)));
    actionProcessSha_toClipboard->setIcon(QIcon(QString(":/icons/%1/copy.svg").arg(theme)));
    actionOpenDatabase->setIcon(QIcon(QString(":/icons/%1/database.svg").arg(theme)));
    actionCheckSumFile->setIcon(QIcon(QString(":/icons/%1/scan.svg").arg(theme)));

    menuAlgo->menuAction()->setIcon(QIcon(QString(":/icons/%1/double-gear.svg").arg(theme)));
    menuStoreSummary->menuAction()->setIcon(QIcon(QString(":/icons/%1/save.svg").arg(theme)));

    // DB Model View
    actionCancelBackToFS->setIcon(QIcon(QString(":/icons/%1/process-stop.svg").arg(theme)));
    actionShowDbStatus->setIcon(QIcon(QString(":/icons/%1/database.svg").arg(theme)));
    actionResetDb->setIcon(QIcon(QString(":/icons/%1/undo.svg").arg(theme)));
    actionForgetChanges->setIcon(QIcon(QString(":/icons/%1/backup.svg").arg(theme)));
    actionUpdateDbWithReChecksums->setIcon(QIcon(QString(":/icons/%1/update.svg").arg(theme)));
    actionUpdateDbWithNewLost->setIcon(QIcon(QString(":/icons/%1/update.svg").arg(theme)));
    actionShowNewLostOnly->setIcon(QIcon(QString(":/icons/%1/newfile.svg").arg(theme)));
    actionShowMismatchesOnly->setIcon(QIcon(QString(":/icons/%1/document-close.svg").arg(theme)));
    actionCheckCurFileFromModel->setIcon(QIcon(QString(":/icons/%1/scan.svg").arg(theme)));
    actionCheckCurSubfolderFromModel->setIcon(QIcon(QString(":/icons/%1/folder-sync.svg").arg(theme)));
    actionCheckAll->setIcon(QIcon(QString(":/icons/%1/start.svg").arg(theme)));
    actionCopyStoredChecksum->setIcon(QIcon(QString(":/icons/%1/copy.svg").arg(theme)));
    actionCopyReChecksum->setIcon(QIcon(QString(":/icons/%1/copy.svg").arg(theme)));
    actionBranchSubfolder->setIcon(QIcon(QString(":/icons/%1/add-fork.svg").arg(theme)));
}

void ModeSelector::processing(bool isProcessing)
{
    if (isProcessing_ != isProcessing) {
        isProcessing_ = isProcessing;
        setMode();

        // when the process is completed, return to the Proxy Model view
        if (!isProcessing && view_->isCurrentViewModel(ModelView::ModelSource)) {
            view_->setTreeModel(ModelView::ModelProxy);

            // if there are Mismatches in the Model, filter them
            if (view_->data_->numbers.numberOf(FileStatus::Mismatched) > 0)
                view_->setFilter(FileStatus::Mismatched);
        }
    }
}

void ModeSelector::prepareView()
{
    if (view_->currentViewModel() == ModelView::ModelProxy) {
        view_->disableFilter(); // if proxy model filtering is enabled, starting a Big Data queuing/verification may be very slow,
                                // even if switching to Source Model, so disable filtering first

        view_->setTreeModel(ModelView::ModelSource); // set the Source Model for the duration of the process,
                                                    // because the Proxy Model is not friendly with Big Data
    }

    processing(true);
}

void ModeSelector::setMode()
{
    if (isProcessing_) {
        button_->setText("Cancel");
        button_->setIcon(QIcon(QString(":/icons/%1/cancel.svg").arg(tools::themeFolder(view_->palette()))));
        button_->setToolTip("");
        return;
    }

    if (view_->isViewFileSystem())
        selectMode(view_->curPathFileSystem);
    else if (view_->isViewDatabase())
        selectMode(view_->data_->numbers);
    else {
        qDebug() << "ModeSelector | Insufficient data to set mode";
        return;
    }

    setButtonInfo();
}

void ModeSelector::setButtonInfo()
{
    button_->setToolTip(QString());
    QString themeFolder = tools::themeFolder(view_->palette());

    switch (curMode) {
    case Folder:
        button_->setText(format::algoToStr(settings_->algorithm));
        button_->setIcon(QIcon(QString(":/icons/%1/folder-sync.svg").arg(themeFolder)));
        button_->setToolTip(QString("Calculate checksums of contained files\nand save the result to the local database"));
        break;
    case File:
        button_->setText(format::algoToStr(settings_->algorithm, false).prepend("*."));
        button_->setIcon(QIcon(QString(":/icons/%1/save.svg").arg(themeFolder)));
        button_->setToolTip(QString("Calculate %1 checksum\nand store it in the summary file").arg(format::algoToStr(settings_->algorithm)));
        break;
    case DbFile:
        button_->setText("Open");
        button_->setIcon(QIcon(QString(":/icons/%1/database.svg").arg(themeFolder)));
        break;
    case SumFile:
        button_->setText("Check");
        button_->setIcon(QIcon(QString(":/icons/%1/scan.svg").arg(themeFolder)));
        break;
    case Model:
        button_->setText("Verify");
        button_->setIcon(QIcon(QString(":/icons/%1/start.svg").arg(themeFolder)));
        button_->setToolTip(QString("Check ALL files against stored checksums"));
        break;
    case ModelNewLost:
        button_->setText("New/Lost");
        button_->setIcon(QIcon(QString(":/icons/%1/update.svg").arg(themeFolder)));
        button_->setToolTip(QString("Update the Database:\nadd new files, delete missing ones"));
        break;
    case UpdateMismatch:
        button_->setText("Update");
        button_->setIcon(QIcon(QString(":/icons/%1/update.svg").arg(themeFolder)));
        button_->setToolTip(QString("Update mismatched checksums with newly calculated ones"));
        break;
    case NoMode:
        button_->setText("Browse");
        break;
    default:
        qDebug() << "ModeSelector::selectMode | WRONG MODE" << curMode;
        break;
    }
}

Mode ModeSelector::selectMode(const QString &path)
{
    QFileInfo pathInfo(path);

    if (pathInfo.isDir())
        curMode = Folder;
    else if (pathInfo.isFile()) {
        if (tools::isDatabaseFile(path))
            curMode = DbFile;
        else if (tools::isSummaryFile(path))
            curMode = SumFile;
        else
            curMode = File;
    }

    emit getPathInfo(path);
    return curMode;
}

Mode ModeSelector::selectMode(const Numbers &numbers)
{
    if (numbers.numberOf(FileStatus::Mismatched) > 0)
        curMode = UpdateMismatch;
    else if (numbers.numberOf(FileStatus::New) > 0 || numbers.numberOf(FileStatus::Missing) > 0)
        curMode = ModelNewLost;
    else
        curMode = Model;

    emit getIndexInfo(view_->curIndexSource);
    return curMode;
}

Mode ModeSelector::currentMode()
{
    return curMode;
}

bool ModeSelector::isCurrentMode(const Mode mode)
{
    return mode == curMode;
}

bool ModeSelector::isProcessing()
{
    return isProcessing_;
}

void ModeSelector::setAlgorithm(QCryptographicHash::Algorithm algo)
{
    settings_->algorithm = algo;
    setButtonInfo();
}

// tasks execution --->>>
void ModeSelector::computeFileChecksum(QCryptographicHash::Algorithm algo, bool summaryFile, bool clipboard)
{
    emit processFileSha(view_->curPathFileSystem, algo, summaryFile, clipboard);
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

void ModeSelector::checkFileChecksum(const QString &checkSum)
{
    emit checkFile(view_->curPathFileSystem, checkSum);
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
    if ((view_->isCurrentViewModel(ModelView::ModelSource) || view_->isCurrentViewModel(ModelView::ModelProxy))
        && view_->curIndexSource.isValid()) {

        QString strData = TreeModel::siblingAtRow(view_->curIndexSource, column).data().toString();
        if (!strData.isEmpty())
            QGuiApplication::clipboard()->setText(strData);
    }
}

void ModeSelector::openFsPath(const QString &path)
{
    // This non-obvious sequence of functions is needed to use
    // the busy state of another [Manager] thread as a timer
    // (to defer data cleanup and prevent segmentation faults)

    if (processAbortPrompt(false)) {
        if (QFileInfo::exists(path))
            view_->curPathFileSystem = path;

        view_->setFileSystemModel();

        if (isProcessing())
            emit cancelProcess();
    }
}

void ModeSelector::openJsonDatabase(const QString &filePath)
{
    if (processAbortPrompt())
        emit parseJsonFile(filePath);
}

void ModeSelector::processFolderChecksums()
{
    processFolderChecksums(settings_->filter);
}

void ModeSelector::processFolderChecksums(const FilterRule &filter)
{
    // if a very large folder is selected, the file system iteration (info about folder contents process) may continue for some time,
    // so cancelation is needed before starting a new process
    emit cancelProcess();

    if (Files::isEmptyFolder(view_->curPathFileSystem)) {
        QMessageBox messageBox;
        messageBox.information(view_, "Empty folder", "Nothing to do.");
        return;
    }

    QString folderName = settings_->addWorkDirToFilename ? paths::basicName(view_->curPathFileSystem) : QString();
    QString databaseFileName = format::composeDbFileName(settings_->dbPrefix, folderName, settings_->databaseFileExtension());

    MetaData metaData;
    metaData.workDir = view_->curPathFileSystem;
    metaData.algorithm = settings_->algorithm;
    metaData.filter = filter;
    metaData.databaseFilePath = paths::joinPath(metaData.workDir, databaseFileName);

    if (!QFileInfo(metaData.databaseFilePath).isFile()) {
        emit processFolderSha(metaData);
        return;
    }

    // if the folder already contains a database file with the same name, display a prompt
    QMessageBox msgBox(view_);
    msgBox.setWindowTitle("Existing database detected");
    msgBox.setText(QString("The folder already contains the database file:\n%1").arg(databaseFileName));
    msgBox.setInformativeText("Do you want to open or overwrite it?");
    msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::Save | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Open);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.button(QMessageBox::Save)->setText("Overwrite");
    int ret = msgBox.exec();

    switch (ret) {
        case QMessageBox::Open:
            emit parseJsonFile(metaData.databaseFilePath);
            break;
        case QMessageBox::Save:
            emit processFolderSha(metaData);
            break;
        default:
            return;
            break;
    }
}

void ModeSelector::processFolderFilteredChecksums()
{
    emit cancelProcess();

    if (Files::isEmptyFolder(view_->curPathFileSystem)) {
            QMessageBox messageBox;
            messageBox.information(view_, "Empty folder", "Nothing to do.");
            return;
    }

    emit makeFolderContentsFilter(view_->curPathFileSystem);
}

void ModeSelector::doWork()
{
    if (isProcessing()) {
        if (view_->data_ && !view_->data_->metaData.isImported)
            showFileSystem();
        else
            emit cancelProcess();
        return;
    }

    switch (curMode) {
        case Folder:
            processFolderChecksums();
            break;
        case File:
            emit processFileSha(view_->curPathFileSystem, settings_->algorithm);
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
            emit updateNewLost();
            break;
        case UpdateMismatch:
            emit updateMismatch();
            break;
        case NoMode:
            showFileSystem();
            break;
        default:
            qDebug() << "MainWindow::doWork() | Wrong MODE:" << curMode;
            break;
    }
}

void ModeSelector::quickAction()
{
    if (isProcessing())
            return;

    switch (curMode) {
        case File:
            emit processFileSha(view_->curPathFileSystem, settings_->algorithm, false, true);
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

        if (isProcessing())
            viewContextMenu->addAction(actionCancel);
        else if (index.isValid()) {
            if (isCurrentMode(Folder)) {
                viewContextMenu->addAction(actionShowFolderContentsTypes);
                viewContextMenu->addMenu(menuAlgorithm());
                viewContextMenu->addSeparator();
                viewContextMenu->addAction(actionProcessContainedChecksums);
                viewContextMenu->addAction(actionProcessFilteredChecksums);
            }
            else if (isCurrentMode(File)) {
                viewContextMenu->addMenu(menuStoreSummary);
                viewContextMenu->addMenu(menuAlgorithm());
                viewContextMenu->addAction(actionProcessSha_toClipboard);

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
        if (isProcessing()) {
            if (view_->data_->metaData.isImported)
                viewContextMenu->addAction(actionCancel);
            viewContextMenu->addAction(actionCancelBackToFS);
        }
        else {
            viewContextMenu->addAction(actionShowDbStatus);
            viewContextMenu->addAction(actionResetDb);
            if (view_->data_ && QFile::exists(view_->data_->backupFilePath()))
                viewContextMenu->addAction(actionForgetChanges);

            viewContextMenu->addSeparator();
            viewContextMenu->addAction(actionShowFilesystem);
            if (view_->isCurrentViewModel(ModelView::ModelProxy)
                && view_->data_->proxyModel_->isFilterEnabled) {
                viewContextMenu->addAction(actionShowAll);
            }
            viewContextMenu->addSeparator();

            if (isCurrentMode(UpdateMismatch)) {
                if (view_->isCurrentViewModel(ModelView::ModelProxy)) {
                    actionShowMismatchesOnly->setChecked(view_->data_->proxyModel_->currentlyFiltered().contains(FileStatus::Mismatched));
                    viewContextMenu->addAction(actionShowMismatchesOnly);
                }
                viewContextMenu->addAction(actionUpdateDbWithReChecksums);
                if (TreeModel::isFileRow(index) && TreeModel::siblingAtRow(index, Column::ColumnReChecksum).data().isValid()) {
                    viewContextMenu->addAction(actionCopyReChecksum);
                }
            }
            else if (isCurrentMode(ModelNewLost)) {
                if (view_->isCurrentViewModel(ModelView::ModelProxy)) {
                    actionShowNewLostOnly->setChecked(view_->data_->proxyModel_->currentlyFiltered().contains(FileStatus::New));
                    viewContextMenu->addAction(actionShowNewLostOnly);
                }

                viewContextMenu->addAction(actionUpdateDbWithNewLost);
                viewContextMenu->addSeparator();
            }

            if (isCurrentMode(Model) || isCurrentMode(ModelNewLost)) {
                if (index.isValid()) {
                    if (TreeModel::isFileRow(index)) {
                        viewContextMenu->addAction(actionCheckCurFileFromModel);
                        viewContextMenu->addAction(actionCopyStoredChecksum);
                    }
                    else if (TreeModel::containsChecksums(index)) {
                        if (!QFileInfo::exists(view_->data_->dbSubFolderDbFilePath(index))) // preventing accidental overwriting
                            viewContextMenu->addAction(actionBranchSubfolder);

                        viewContextMenu->addAction(actionCheckCurSubfolderFromModel);
                    }
                }
                viewContextMenu->addAction(actionCheckAll);
            }
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

    QIcon dbIcon = QIcon(QString(":/icons/%1/database.svg").arg(tools::themeFolder(view_->palette())));

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
    switch (settings_->algorithm) {
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

    menuAlgo->menuAction()->setText("Algorithm " + format::algoToStr(settings_->algorithm));

    return menuAlgo;
}

void ModeSelector::createContextMenu_Button(const QPoint &point)
{
    if (!isProcessing() && (isCurrentMode(File) || isCurrentMode(Folder)))
        menuAlgorithm()->exec(button_->mapToGlobal(point));
}

bool ModeSelector::processAbortPrompt(const bool sendCancelation)
{
    if (!isProcessing())
        return true;

    int ret = QMessageBox::question(view_, "Processing...", "Abort current process?", QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        if (sendCancelation)
            emit cancelProcess();
        return true;
    }
    else
        return false;
}
