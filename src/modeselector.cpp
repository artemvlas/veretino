// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "modeselector.h"
#include <QFileInfo>
#include <QDebug>

ModeSelector::ModeSelector(View *view, Settings *settings, QObject *parent)
    : QObject{parent}, view_(view), settings_(settings)
{
    connect(this, &ModeSelector::getPathInfo, this, &ModeSelector::cancelProcess);
    connect(this, &ModeSelector::folderContentsByType, this, &ModeSelector::cancelProcess);

    connect(this, &ModeSelector::updateNewLost, this, &ModeSelector::prepareView);
    connect(this, &ModeSelector::updateMismatch, this, &ModeSelector::prepareView);
    connect(this, &ModeSelector::verify, this, [=](const QModelIndex &ind){if (!TreeModel::isFileRow(ind)) prepareView();});
}

void ModeSelector::processing(bool isProcessing)
{
    if (isProcessing_ != isProcessing) {
        isProcessing_ = isProcessing;
        setMode();

        // when the process is completed, return to the Proxy Model view
        if (!isProcessing && view_->currentViewModel() == ModelView::ModelSource)
            view_->setTreeModel(ModelView::ModelProxy);
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

void ModeSelector::copyStoredChecksumToClipboard()
{
    emit copyStoredChecksum(view_->curIndexSource);
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
