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

ModeSelector::ModeSelector(View *view, Settings *settings, QObject *parent)
    : QObject{parent}, view_(view), settings_(settings)
{
    connect(this, &ModeSelector::getPathInfo, this, &ModeSelector::cancelProcess);
    connect(this, &ModeSelector::makeFolderContentsList, this, &ModeSelector::cancelProcess);
    connect(this, &ModeSelector::makeFolderContentsFilter, this, &ModeSelector::cancelProcess);

    connect(this, &ModeSelector::updateDatabase, view_, &View::setViewSource);
    connect(this, &ModeSelector::verify, this, [=](const QModelIndex &ind){ if (!TreeModel::isFileRow(ind)) view_->setViewSource(); });

    connect(this, &ModeSelector::resetDatabase, view_, &View::saveHeaderState);

    iconProvider.setTheme(view_->palette());
    menuAct_->setIconTheme(view_->palette());

    connectActions();
}

void ModeSelector::connectActions()
{
    // MainWindow menu
    connect(menuAct_->actionShowFilesystem, &QAction::triggered, this, &ModeSelector::showFileSystem);
    connect(menuAct_->actionClearRecent, &QAction::triggered, settings_, &Settings::clearRecentFiles);

    // File system View
    connect(menuAct_->actionToHome, &QAction::triggered, view_, &View::toHome);
    connect(menuAct_->actionCancel, &QAction::triggered, this, &ModeSelector::cancelProcess);
    connect(menuAct_->actionShowFolderContentsTypes, &QAction::triggered, this, &ModeSelector::showFolderContentTypes);
    connect(menuAct_->actionProcessChecksumsNoFilter, &QAction::triggered, this, &ModeSelector::processChecksumsNoFilter);
    connect(menuAct_->actionProcessChecksumsPermFilter, &QAction::triggered, this, &ModeSelector::processChecksumsPermFilter);
    connect(menuAct_->actionProcessChecksumsCustomFilter, &QAction::triggered, this, &ModeSelector::processChecksumsFiltered);
    connect(menuAct_->actionCheckFileByClipboardChecksum, &QAction::triggered, this, &ModeSelector::checkFileByClipboardChecksum);
    connect(menuAct_->actionProcessSha1File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha1); });
    connect(menuAct_->actionProcessSha256File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha256); });
    connect(menuAct_->actionProcessSha512File, &QAction::triggered, this, [=]{ procSumFile(QCryptographicHash::Sha512); });
    connect(menuAct_->actionProcessSha_toClipboard, &QAction::triggered, this,
            [=]{ emit processFileSha(view_->curPathFileSystem, settings_->algorithm(), DestFileProc::Clipboard); });
    connect(menuAct_->actionOpenDatabase, &QAction::triggered, this, &ModeSelector::doWork);
    connect(menuAct_->actionCheckSumFile , &QAction::triggered, this, &ModeSelector::doWork);

    // DB Model View
    connect(menuAct_->actionCancelBackToFS, &QAction::triggered, this, &ModeSelector::showFileSystem);
    connect(menuAct_->actionShowDbStatus, &QAction::triggered, view_, &View::showDbStatus);
    connect(menuAct_->actionResetDb, &QAction::triggered, this, &ModeSelector::resetDatabase);
    connect(menuAct_->actionForgetChanges, &QAction::triggered, this, &ModeSelector::restoreDatabase);
    connect(menuAct_->actionUpdateDbWithReChecksums, &QAction::triggered, this, [=]{ emit updateDatabase(DestDbUpdate::DestUpdateMismatches); });
    connect(menuAct_->actionUpdateDbWithNewLost, &QAction::triggered, this, [=]{ emit updateDatabase(DestDbUpdate::DestUpdateNewLost); });
    connect(menuAct_->actionDbAddNew, &QAction::triggered, this, [=]{ emit updateDatabase(DestDbUpdate::DestAddNew); });
    connect(menuAct_->actionDbClearLost, &QAction::triggered, this, [=]{ emit updateDatabase(DestDbUpdate::DestClearLost); });
    connect(menuAct_->actionFilterNewLost, &QAction::triggered, this,
            [=](bool isChecked){ view_->editFilter(FileStatus::FlagNewLost, isChecked); });
    connect(menuAct_->actionFilterMismatches, &QAction::triggered, this,
            [=](bool isChecked){ view_->editFilter(FileStatus::Mismatched, isChecked); });
    connect(menuAct_->actionShowAll, &QAction::triggered, view_, &View::disableFilter);
    connect(menuAct_->actionCheckCurFileFromModel, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(menuAct_->actionCheckCurSubfolderFromModel, &QAction::triggered, this, &ModeSelector::verifyItem);
    connect(menuAct_->actionCheckAll, &QAction::triggered, this, &ModeSelector::verifyDb);
    connect(menuAct_->actionCopyStoredChecksum, &QAction::triggered, this, [=]{ copyDataToClipboard(Column::ColumnChecksum); });
    connect(menuAct_->actionCopyReChecksum, &QAction::triggered, this, [=]{ copyDataToClipboard(Column::ColumnReChecksum); });
    connect(menuAct_->actionCopyItem, &QAction::triggered, this, &ModeSelector::copyItem);
    connect(menuAct_->actionBranchMake, &QAction::triggered, this, [=]{ emit branchSubfolder(view_->curIndexSource); });
    connect(menuAct_->actionBranchOpen, &QAction::triggered, this, &ModeSelector::openBranchDb);

    connect(menuAct_->actionCollapseAll, &QAction::triggered, view_, &View::collapseAll);
    connect(menuAct_->actionExpandAll, &QAction::triggered, view_, &View::expandAll);

    // Algorithm selection
    connect(menuAct_->actionSetAlgoSha1, &QAction::triggered, this, [=]{ settings_->setAlgorithm(QCryptographicHash::Sha1); });
    connect(menuAct_->actionSetAlgoSha256, &QAction::triggered, this, [=]{ settings_->setAlgorithm(QCryptographicHash::Sha256); });
    connect(menuAct_->actionSetAlgoSha512, &QAction::triggered, this, [=]{ settings_->setAlgorithm(QCryptographicHash::Sha512); });

    // recent files menu
    connect(menuAct_->menuOpenRecent, &QMenu::triggered, this, &ModeSelector::openRecentDatabase);
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
    switch (mode()) {
        case Processing:
            return "Cancel";
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
        case Processing:
            return iconProvider.icon(Icons::Cancel);
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

Mode ModeSelector::mode()
{
    if (proc_->isState(State::StartVerbose) || view_->isCurrentViewModel(ModelView::ModelSource))
        return Processing;

    if (view_->isViewDatabase()) {
        if (view_->data_->numbers.contains(FileStatus::Mismatched))
            return UpdateMismatch;
        else if (view_->data_->numbers.contains(FileStatus::FlagNewLost))
            return ModelNewLost;
        else
            return Model;
    }

    if (view_->isViewFileSystem()) {
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
    switch (mode()) {
        case Processing:
            if (view_->data_ && !view_->data_->metaData.isImported)
                showFileSystem();
            else
                cancelProcess();
            break;
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
        viewContextMenu->addAction(menuAct_->actionShowFilesystem);
    }
    // Filesystem View
    else if (view_->isViewFileSystem()) {
        viewContextMenu->addAction(menuAct_->actionToHome);
        viewContextMenu->addSeparator();

        if (proc_->isState(State::StartVerbose))
            viewContextMenu->addAction(menuAct_->actionCancel);
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
    }
    // TreeModel or ProxyModel View
    else if (view_->isViewDatabase()) {
        if (proc_->isStarted()) {
            if (view_->data_->metaData.isImported)
                viewContextMenu->addAction(menuAct_->actionCancel);
            viewContextMenu->addAction(menuAct_->actionCancelBackToFS);
        }
        else {
            viewContextMenu->addAction(menuAct_->actionShowDbStatus);
            viewContextMenu->addAction(menuAct_->actionResetDb);
            if (QFile::exists(view_->data_->backupFilePath()))
                viewContextMenu->addAction(menuAct_->actionForgetChanges);

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

                if (view_->data_->numbers.contains(FileStatus::FlagNewLost)) {
                    menuAct_->actionFilterNewLost->setChecked(view_->isViewFiltered(FileStatus::New));
                    viewContextMenu->addAction(menuAct_->actionFilterNewLost);
                }

                if (view_->data_->numbers.contains(FileStatus::FlagUpdatable))
                    viewContextMenu->addSeparator();
            }

            if (index.isValid()) {
                 if (TreeModel::isFileRow(index)) {
                    if (TreeModel::hasReChecksum(index))
                        viewContextMenu->addAction(menuAct_->actionCopyReChecksum);
                    else if (TreeModel::hasChecksum(index))
                        viewContextMenu->addAction(menuAct_->actionCopyStoredChecksum);

                    if (TreeModel::hasStatus(FileStatus::FlagAvailable, index)) {
                        menuAct_->actionCopyItem->setText("Copy File");
                        viewContextMenu->addAction(menuAct_->actionCopyItem);

                        viewContextMenu->addAction(menuAct_->actionCheckCurFileFromModel);
                    }
                }
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
    }

    viewContextMenu->exec(view_->viewport()->mapToGlobal(point));
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
