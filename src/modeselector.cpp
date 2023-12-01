// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "modeselector.h"
#include <QFileInfo>
#include <QGuiApplication>
#include <QClipboard>
#include <QMenu>
#include <QDebug>

ModeSelector::ModeSelector(View *view, Settings *settings, QObject *parent)
    : QObject{parent}, view_(view), settings_(settings)
{
    actionShowNewLostOnly->setCheckable(true);
    actionShowMismatchesOnly->setCheckable(true);

    connect(this, &ModeSelector::getPathInfo, this, &ModeSelector::cancelProcess);
    connect(this, &ModeSelector::folderContentsByType, this, &ModeSelector::cancelProcess);

    connect(this, &ModeSelector::updateNewLost, this, &ModeSelector::prepareView);
    connect(this, &ModeSelector::updateMismatch, this, &ModeSelector::prepareView);
    connect(this, &ModeSelector::verify, this, [=](const QModelIndex &ind){if (!TreeModel::isFileRow(ind)) prepareView();});

    connectActions();
}

void ModeSelector::connectActions()
{
    connect(actionShowFS, &QAction::triggered, this, &ModeSelector::showFileSystem);
    connect(actionToHome, &QAction::triggered, view_, &View::toHome);
    connect(actionCancel, &QAction::triggered, this, &ModeSelector::cancelProcess);
    connect(actionShowFolderContentsTypes, &QAction::triggered, this, &ModeSelector::showFolderContentTypes);
    connect(actionProcessFolderChecksums, &QAction::triggered, this, &ModeSelector::doWork);
    connect(actionCheckFileByClipboardChecksum, &QAction::triggered, this, [=]{checkFileChecksum(QGuiApplication::clipboard()->text());});
    connect(actionProcessSha1File, &QAction::triggered, this, [=]{computeFileChecksum(QCryptographicHash::Sha1);});
    connect(actionProcessSha256File, &QAction::triggered, this, [=]{computeFileChecksum(QCryptographicHash::Sha256);});
    connect(actionProcessSha512File, &QAction::triggered, this, [=]{computeFileChecksum(QCryptographicHash::Sha512);});
    connect(actionOpenDatabase, &QAction::triggered, this, &ModeSelector::doWork);
    connect(actionCheckSumFile , &QAction::triggered, this, &ModeSelector::doWork);

    connect(actionCancelBackToFS, &QAction::triggered, this, &ModeSelector::showFileSystem);
    connect(actionShowDbStatus, &QAction::triggered, this, &ModeSelector::dbStatus);
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

    connect(actionCollapseAll, &QAction::triggered, view_, &View::collapseAll);
    connect(actionExpandAll, &QAction::triggered, view_, &View::expandAll);
}

void ModeSelector::processing(bool isProcessing)
{
    if (isProcessing_ != isProcessing) {
        isProcessing_ = isProcessing;
        setMode();

        // when the process is completed, return to the Proxy Model view
        if (!isProcessing && view_->currentViewModel() == ModelView::ModelSource) {
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
}

void ModeSelector::setMode()
{
    if (isProcessing_) {
        emit setButtonText("Cancel");
        return;
    }

    if (view_->isViewFileSystem())
        selectMode(view_->curPathFileSystem);
    else if (view_->data_)
        selectMode(view_->data_->numbers);
    else {
        qDebug() << "ModeSelector | Insufficient data to set mode";
        return;
    }

    switch (curMode) {
        case Folder:
            emit setButtonText(format::algoToStr(settings_->algorithm).append(": Folder"));
            break;
        case File:
            emit setButtonText(format::algoToStr(settings_->algorithm).append(": File"));
            break;
        case DbFile:
            emit setButtonText("Open DataBase");
            break;
        case SumFile:
            emit setButtonText("Check");
            break;
        case Model:
            emit setButtonText("Verify All");
            break;
        case ModelNewLost:
            emit setButtonText("Update New/Lost");
            break;
        case UpdateMismatch:
            emit setButtonText("Update");
            break;
        case NoMode:
            emit setButtonText("Browse");
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

bool ModeSelector::isProcessing()
{
    return isProcessing_;
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
    emit folderContentsByType(view_->curPathFileSystem);
}

void ModeSelector::checkFileChecksum(const QString &checkSum)
{
    emit checkFile(view_->curPathFileSystem, checkSum);
}

void ModeSelector::showFileSystem()
{
    if (isProcessing())
        emit cancelProcess();

    view_->setFileSystemModel();
}

void ModeSelector::copyDataToClipboard(Column column)
{
    if ((view_->currentViewModel() == ModelView::ModelSource || view_->currentViewModel() == ModelView::ModelProxy)
        && view_->curIndexSource.isValid()) {

        QString strData = TreeModel::siblingAtRow(view_->curIndexSource, column).data().toString();
        if (!strData.isEmpty())
            QGuiApplication::clipboard()->setText(strData);
    }
}

void ModeSelector::doWork()
{
    if (isProcessing()) {
        emit cancelProcess();
        return;
    }

    switch (curMode) {
        case Folder:
            // if a very large folder is selected, the file system iteration (info about folder contents process) may continue for some time,
            // so cancelation is needed before starting a new process
            emit cancelProcess();
            emit processFolderSha(view_->curPathFileSystem, settings_->algorithm);
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

    if (view_->currentViewModel() == ModelView::NotSetted) {
        viewContextMenu->addAction(actionShowFS);
    }
    // Filesystem View
    else if (view_->isViewFileSystem()) {
        viewContextMenu->addAction(actionToHome);
        viewContextMenu->addSeparator();

        if (isProcessing())
            viewContextMenu->addAction(actionCancel);
        else if (index.isValid()) {
            if (currentMode() == ModeSelector::Folder) {
                viewContextMenu->addAction(actionShowFolderContentsTypes);
                viewContextMenu->addSeparator();
                actionProcessFolderChecksums->setText(QString("Compute %1 for all files in folder")
                                                     .arg(format::algoToStr(settings_->algorithm)));
                viewContextMenu->addAction(actionProcessFolderChecksums);
            }
            else if (currentMode() == ModeSelector::File) {
                QString clipboardText = QGuiApplication::clipboard()->text();
                if (tools::canBeChecksum(clipboardText)) {
                    actionCheckFileByClipboardChecksum->setText("Check the file by checksum: " + format::shortenString(clipboardText));
                    viewContextMenu->addAction(actionCheckFileByClipboardChecksum);
                    viewContextMenu->addSeparator();
                }
                viewContextMenu->addAction(actionProcessSha1File);
                viewContextMenu->addAction(actionProcessSha256File);
                viewContextMenu->addAction(actionProcessSha512File);
            }
            else if (currentMode() == ModeSelector::DbFile)
                viewContextMenu->addAction(actionOpenDatabase);
            else if (currentMode() == ModeSelector::SumFile)
                viewContextMenu->addAction(actionCheckSumFile);
        }
    }
    // TreeModel or ProxyModel View
    else {
        if (isProcessing()) {
            viewContextMenu->addAction(actionCancel);
            viewContextMenu->addAction(actionCancelBackToFS);
        }
        else {
            viewContextMenu->addAction(actionShowDbStatus);
            viewContextMenu->addAction(actionResetDb);
            if (view_->data_ && QFile::exists(view_->data_->backupFilePath()))
                viewContextMenu->addAction(actionForgetChanges);

            viewContextMenu->addSeparator();
            viewContextMenu->addAction(actionShowFS);
            if (view_->currentViewModel() == ModelView::ModelProxy
                && view_->data_->proxyModel_->isFilterEnabled) {
                viewContextMenu->addAction(actionShowAll);
            }
            viewContextMenu->addSeparator();

            if (currentMode() == ModeSelector::UpdateMismatch) {
                if (view_->currentViewModel() == ModelView::ModelProxy) {
                    actionShowMismatchesOnly->setChecked(view_->data_->proxyModel_->currentlyFiltered().contains(FileStatus::Mismatched));
                    viewContextMenu->addAction(actionShowMismatchesOnly);
                }
                viewContextMenu->addAction(actionUpdateDbWithReChecksums);
                if (TreeModel::isFileRow(index) && TreeModel::siblingAtRow(index, Column::ColumnReChecksum).data().isValid()) {
                    viewContextMenu->addAction(actionCopyReChecksum);
                }
            }
            else if (currentMode() == ModeSelector::ModelNewLost) {
                if (view_->currentViewModel() == ModelView::ModelProxy) {
                    actionShowNewLostOnly->setChecked(view_->data_->proxyModel_->currentlyFiltered().contains(FileStatus::New));
                    viewContextMenu->addAction(actionShowNewLostOnly);
                }

                viewContextMenu->addAction(actionUpdateDbWithNewLost);
                viewContextMenu->addSeparator();
            }

            if (currentMode() == ModeSelector::Model || currentMode() == ModeSelector::ModelNewLost) {
                if (index.isValid()) {
                    if (TreeModel::isFileRow(index)) {
                        viewContextMenu->addAction(actionCheckCurFileFromModel);
                        viewContextMenu->addAction(actionCopyStoredChecksum);
                    }
                    else
                        viewContextMenu->addAction(actionCheckCurSubfolderFromModel);
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
