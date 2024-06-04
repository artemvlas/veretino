/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dialogdbstatus.h"
#include "dialogfoldercontents.h"
#include "dialogfileprocresult.h"
#include "dialogsettings.h"
#include "dialogabout.h"
#include <QMimeData>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(IconProvider::appIcon());
    QThread::currentThread()->setObjectName("MAIN Thread");

    ui->treeView->setSettings(settings_);
    settings_->loadSettings();

    ui->progressBar->setProcState(manager->procState);
    ui->progressBar->setVisible(false);

    restoreGeometry(settings_->geometryMainWindow);

    modeSelect = new ModeSelector(ui->treeView, settings_, this);
    modeSelect->setProcState(manager->procState);
    proc_ = manager->procState;

    statusTextLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::MinimumExpanding);
    statusIconLabel->setContentsMargins(5, 0, 0, 0);
    permanentStatus->setContentsMargins(20, 0, 0, 0);
    ui->statusbar->addWidget(statusIconLabel);
    ui->statusbar->addWidget(statusTextLabel, 1);
    ui->statusbar->addPermanentWidget(permanentStatus);

    connections();

    if (!argumentInput())
        ui->treeView->setFileSystemModel();

    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->button->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->menuFile->addActions(modeSelect->menuAct_->menuFileActions);
    ui->menuFile->insertSeparator(modeSelect->menuAct_->actionOpenDialogSettings);
    ui->menuFile->insertMenu(modeSelect->menuAct_->actionSave, modeSelect->menuAct_->menuOpenRecent);

    updatePermanentStatus();
}

MainWindow::~MainWindow()
{
    //proc_->setState(State::Abort);
    emit modeSelect->saveData();
    saveSettings();

    thread->quit();
    thread->wait();

    delete ui;
}

// if a computing process is running, show a hint when user wants to close the app
void MainWindow::closeEvent(QCloseEvent *event)
{
    modeSelect->processAbortPrompt() ? event->accept()
                                     : event->ignore();
}

void MainWindow::connections()
{
    connectManager();

    // Push Button
    connect(ui->button, &QPushButton::clicked, modeSelect, &ModeSelector::doWork);
    connect(ui->button, &QPushButton::customContextMenuRequested, this, &MainWindow::createContextMenu_Button);

    connect(settings_, &Settings::algorithmChanged, this, &MainWindow::updateButtonInfo);

    // TreeView
    connect(ui->treeView, &View::keyEnterPressed, modeSelect, &ModeSelector::quickAction);
    connect(ui->treeView, &View::doubleClicked, modeSelect, &ModeSelector::quickAction);
    connect(ui->treeView, &View::customContextMenuRequested, modeSelect, &ModeSelector::createContextMenu_View);
    connect(ui->treeView, &View::pathChanged, ui->pathEdit, &QLineEdit::setText);
    connect(ui->treeView, &View::pathChanged, modeSelect, &ModeSelector::getInfoPathItem);
    connect(ui->treeView, &View::pathChanged, this, &MainWindow::updateButtonInfo);
    connect(ui->treeView, &View::modelChanged, this, &MainWindow::handleChangedModel);
    connect(ui->treeView, &View::modelChanged, this, &MainWindow::updatePermanentStatus);
    connect(ui->treeView, &View::showMessage, this, &MainWindow::showMessage);
    connect(ui->treeView, &View::showDbStatus, this, &MainWindow::showDbStatus);

    connect(ui->pathEdit, &QLineEdit::returnPressed, this, &MainWindow::handlePathEdit);
    connect(permanentStatus, &ClickableLabel::clicked, this, &MainWindow::handlePermanentStatusClick);

    // menu actions
    connect(modeSelect->menuAct_->actionOpenDialogSettings, &QAction::triggered, this, &MainWindow::dialogSettings);
    connect(modeSelect->menuAct_->actionOpenFolder, &QAction::triggered, this, &MainWindow::dialogOpenFolder);
    connect(modeSelect->menuAct_->actionOpenDatabaseFile, &QAction::triggered, this, &MainWindow::dialogOpenJson);
    connect(ui->actionAbout, &QAction::triggered, this, [=]{ DialogAbout about(this); about.exec(); });

    // !!! will be reimplement before the next release:
    connect(ui->menuFile, &QMenu::aboutToShow, modeSelect->menuAct_,
            [=] { modeSelect->menuAct_->updateMenuOpenRecent(settings_->recentFiles);
                    modeSelect->menuAct_->actionSave->setEnabled(ui->treeView->data_
                    && ui->treeView->data_->isDbFileState(MetaData::NotSaved)); });

    connect(modeSelect, &ModeSelector::prepareSwitchToFs, manager, &Manager::prepareSwitchToFs);
    connect(manager, &Manager::switchToFsPrepared, ui->treeView, &View::setFileSystemModel); // TMP !!!!!
}

void MainWindow::connectManager()
{
    qRegisterMetaType<QVector<int>>("QVector<int>"); // for building on Windows (qt 5.15.2)
    qRegisterMetaType<QCryptographicHash::Algorithm>("QCryptographicHash::Algorithm");
    qRegisterMetaType<ModelView>("ModelView");
    qRegisterMetaType<QList<ExtNumSize>>("QList<ExtNumSize>");
    qRegisterMetaType<MetaData>("MetaData");
    qRegisterMetaType<Numbers>("Numbers");
    qRegisterMetaType<FileValues>("FileValues");
    qRegisterMetaType<DestFileProc>("DestFileProc");
    qRegisterMetaType<DestDbUpdate>("DestDbUpdate");

    manager->moveToThread(thread);

    // when closing the thread
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(thread, &QThread::finished, manager, &Manager::deleteLater);

    // signals for execution tasks
    connect(modeSelect, &ModeSelector::parseJsonFile, manager, &Manager::createDataModel);
    connect(modeSelect, &ModeSelector::processFolderSha, manager, &Manager::processFolderSha);
    connect(modeSelect, &ModeSelector::processFileSha, manager, &Manager::processFileSha);
    connect(modeSelect, &ModeSelector::verify, manager, &Manager::verify);
    connect(modeSelect, &ModeSelector::updateDatabase, manager, &Manager::updateDatabase);
    connect(modeSelect, &ModeSelector::checkSummaryFile, manager, &Manager::checkSummaryFile); // check *.sha1 *.sha256 *.sha512 summaries
    connect(modeSelect, &ModeSelector::checkFile, manager, qOverload<const QString&, const QString&>(&Manager::checkFile));
    connect(modeSelect, &ModeSelector::branchSubfolder, manager, &Manager::branchSubfolder);
    connect(modeSelect, &ModeSelector::saveData, manager, &Manager::saveData);

    // info and notifications
    connect(manager, &Manager::setStatusbarText, statusTextLabel, &ClickableLabel::setText);
    connect(manager, &Manager::setStatusbarText, this, &MainWindow::updateStatusIcon);
    connect(manager, &Manager::showMessage, this, &MainWindow::showMessage);
    connect(manager, &Manager::folderChecked, ui->treeView, &View::setMismatchFiltering);
    connect(manager, &Manager::folderChecked, this, &MainWindow::showFolderCheckResult);
    connect(manager, &Manager::fileProcessed, this, &MainWindow::showFileCheckResult);
    connect(modeSelect, &ModeSelector::getPathInfo, manager, &Manager::getPathInfo);
    connect(modeSelect, &ModeSelector::getIndexInfo, manager, &Manager::getIndexInfo);
    connect(modeSelect, &ModeSelector::makeFolderContentsList, manager, &Manager::makeFolderContentsList);
    connect(modeSelect, &ModeSelector::makeFolderContentsFilter, manager, &Manager::makeFolderContentsFilter);
    connect(manager, &Manager::folderContentsListCreated, this, &MainWindow::showDialogFolderContents);
    connect(manager, &Manager::folderContentsFilterCreated, this, &MainWindow::showFilterCreationDialog);
    connect(manager, &Manager::finishedCalcFileChecksum, modeSelect, &ModeSelector::getInfoPathItem);

    // results processing
    connect(manager, &Manager::setTreeModel, ui->treeView, &View::setTreeModel);
    connect(manager, &Manager::setViewData, ui->treeView, &View::setData);
    connect(manager->dataMaintainer, &DataMaintainer::databaseUpdated, this, &MainWindow::showDbStatus);
    connect(manager->dataMaintainer, &DataMaintainer::numbersUpdated, this, &MainWindow::updatePermanentStatus);
    connect(manager->dataMaintainer, &DataMaintainer::subDbForked, this, &MainWindow::promptOpenBranch);

    // process status
    connect(manager->procState, &ProcState::stateChanged, this, [=]{ if (proc_->isState(State::Idle)) ui->treeView->setViewProxy(); });
    connect(manager->procState, &ProcState::stateChanged, this, &MainWindow::updateButtonInfo);
    connect(manager->procState, &ProcState::progressStarted, ui->progressBar, &ProgressBar::start);
    connect(manager->procState, &ProcState::progressFinished, ui->progressBar, &ProgressBar::finish);
    connect(manager->procState, &ProcState::percentageChanged, ui->progressBar, &ProgressBar::setValue);

    // change view
    connect(modeSelect, &ModeSelector::resetDatabase, manager, &Manager::resetDatabase); // reopening and reparsing current database
    connect(modeSelect, &ModeSelector::restoreDatabase, manager, &Manager::restoreDatabase);
    connect(ui->treeView, &View::switchedToFs, manager->dataMaintainer, &DataMaintainer::clearData);
    connect(ui->treeView, &View::modelChanged, manager, &Manager::modelChanged);
    connect(ui->treeView, &View::dataSetted, manager->dataMaintainer, &DataMaintainer::clearOldData);
    connect(ui->treeView, &View::dataSetted, this,
            [=]{ if (ui->treeView->data_) settings_->addRecentFile(ui->treeView->data_->metaData.databaseFilePath); });

    thread->start();
}

void MainWindow::saveSettings()
{
    ui->treeView->saveHeaderState();
    settings_->geometryMainWindow = saveGeometry();
    settings_->saveSettings();
}

void MainWindow::showDbStatus()
{
    if (ui->treeView->data_) {
        DialogDbStatus statusDialog(ui->treeView->data_, this);
        statusDialog.exec();
    }
}

void MainWindow::showDialogFolderContents(const QString &folderName, const QList<ExtNumSize> &extList)
{
    if (!extList.isEmpty()) {
        DialogFolderContents dialog(folderName, extList, this);
        if (dialog.exec() == QDialog::Accepted) {
            FilterRule filter = dialog.resultFilter();
            if (!filter.isFilter(FilterRule::NotSet)) {
                settings_->filter = filter;
                updatePermanentStatus();
            }
        }
    }
}

void MainWindow::showFilterCreationDialog(const QString &folderName, const QList<ExtNumSize> &extList)
{
    if (!extList.isEmpty()) {
        DialogFolderContents dialog(folderName, extList, this);
        dialog.setFilterCreatingEnabled();
        FilterRule filter;

        if (dialog.exec() == QDialog::Accepted) {
            filter = dialog.resultFilter();
        }

        if (!filter.isFilter(FilterRule::NotSet)) {
            modeSelect->processFolderChecksums(filter);
        }
        else {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("No filter specified");
            msgBox.setText("File filtering is not set.");
            msgBox.setInformativeText("Continue for all files?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Abort);
            msgBox.setDefaultButton(QMessageBox::Yes);
            msgBox.setIcon(QMessageBox::Question);
            msgBox.button(QMessageBox::Yes)->setText("Continue");

            if (msgBox.exec() == QMessageBox::Yes)
                modeSelect->processFolderChecksums(filter);
        }
    }
}

void MainWindow::showFolderCheckResult(const Numbers &result, const QString &subFolder)
{
    QMessageBox msgBox(this);

    QString titleText = (result.contains(FileStatus::Mismatched)) ? "FAILED" : "Success";
    QString messageText = !subFolder.isEmpty() ? QString("Subfolder: %1\n\n").arg(subFolder) : QString();

    QIcon icon = (result.contains(FileStatus::Mismatched)) ? modeSelect->iconProvider.icon(FileStatus::Mismatched)
                                                           : modeSelect->iconProvider.icon(FileStatus::Matched);

    if (result.contains(FileStatus::Mismatched)) {
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.button(QMessageBox::Ok)->setText("Update");
        msgBox.button(QMessageBox::Cancel)->setText("Continue");
        msgBox.button(QMessageBox::Ok)->setIcon(modeSelect->iconProvider.icon(FileStatus::Updated));
        msgBox.button(QMessageBox::Cancel)->setIcon(QIcon());

        messageText.append(QString("%1 out of %2 files %3 changed or corrupted.")
                            .arg(result.numberOf(FileStatus::Mismatched))
                            .arg(result.numberOf(FileStatus::FlagAvailable))
                            .arg(result.numberOf(FileStatus::Mismatched) == 1 ? "is" : "are"));
    }
    else {
        messageText.append(QString("ALL %1 files passed verification.")
                            .arg(result.numberOf(FileStatus::Matched)));
    }

    msgBox.setWindowTitle(titleText);
    msgBox.setText(messageText);
    msgBox.setIconPixmap(icon.pixmap(64, 64));

    int ret = msgBox.exec();
    if (result.contains(FileStatus::Mismatched) && ret == QMessageBox::Ok) {
        emit modeSelect->updateDatabase(DestDbUpdate::DestUpdateMismatches);
    }
}

void MainWindow::showFileCheckResult(const QString &filePath, const FileValues &values)
{
    DialogFileProcResult dialog(filePath, values, this);
    dialog.exec();
}

void MainWindow::dialogSettings()
{
    DialogSettings dialog(settings_, this);

    if (dialog.exec() == QDialog::Accepted) {
        dialog.updateSettings();
        updatePermanentStatus();
    }
}

void MainWindow::dialogOpenFolder()
{
    QString path = QFileDialog::getExistingDirectory(this, "Open folder", QDir::homePath());

    if (!path.isEmpty()) {
        modeSelect->openFsPath(path);
    }
}

void MainWindow::dialogOpenJson()
{
    QString path = QFileDialog::getOpenFileName(this, "Open Veretino database", QDir::homePath(), "Veretino database (*.ver *.ver.json)");

    if (!path.isEmpty()) {
        modeSelect->openJsonDatabase(path);
    }
}

void MainWindow::showMessage(const QString &message, const QString &title)
{
    QMessageBox messageBox;

    if (title.toLower() == "error" || title.toLower() == "failed")
        messageBox.critical(this, title, message);
    else if (title.toLower() == "warning")
        messageBox.warning(this, title, message);
    else
        messageBox.information(this, title, message);
}

void MainWindow::promptOpenBranch(const QString &dbFilePath)
{
    if (!tools::isDatabaseFile(dbFilePath))
        return;

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("A new Branch has been created");
    msgBox.setText(QString("The subfolder data is forked:\n%1").arg(".../" + paths::basicName(dbFilePath)));
    msgBox.setInformativeText("Do you want to open it or stay in the current one?");
    msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setIcon(QMessageBox::Question);
    msgBox.button(QMessageBox::No)->setText("Stay");

    if (msgBox.exec() == QMessageBox::Open) {
        modeSelect->openJsonDatabase(dbFilePath);
    }
}

void MainWindow::updateButtonInfo()
{
    ui->button->setIcon(modeSelect->getButtonIcon());
    ui->button->setText(modeSelect->getButtonText());
    ui->button->setToolTip(modeSelect->getButtonToolTip());
}

void MainWindow::updateStatusIcon()
{
    QIcon statusIcon;
    if (proc_->isStarted()) {
        statusIcon = modeSelect->iconProvider.icon(FileStatus::Calculating);
    }
    else if (ui->treeView->isViewFileSystem()) {
        statusIcon = modeSelect->iconProvider.icon(ui->treeView->curPathFileSystem);
    }
    else if (ui->treeView->isViewDatabase()) {
        if (TreeModel::isFileRow(ui->treeView->curIndexSource))
            statusIcon = modeSelect->iconProvider.icon(TreeModel::itemFileStatus(ui->treeView->curIndexSource));
        else
            statusIcon = modeSelect->iconProvider.iconFolder();
    }

    statusIconLabel->setPixmap(statusIcon.pixmap(16, 16));
}

void MainWindow::updatePermanentStatus()
{
    if (ui->treeView->isViewDatabase()) {
        if (proc_->isStarted() && ui->treeView->data_->isInCreation()) {
            QString permStatus = format::algoToStr(ui->treeView->data_->metaData.algorithm);
            if (ui->treeView->data_->isFilterApplied())
                permStatus.prepend("filters applied | ");
            permanentStatus->setText(permStatus);
        }
        else
            permanentStatus->setText(getDatabaseStatusSummary());
    }
    else if (ui->treeView->isViewFileSystem()) {
        settings_->filter.extensionsList.isEmpty() ? permanentStatus->clear()
                                                   : permanentStatus->setPixmap(modeSelect->iconProvider.icon(Icons::Filter).pixmap(16, 16));
    }
    else
        permanentStatus->clear();
}

QString MainWindow::getDatabaseStatusSummary()
{
    if (!ui->treeView->data_)
        return QString();

    const Numbers &numbers = ui->treeView->data_->numbers;
    QString checkStatus;
    static QString availNumber;
    static QString availSize;

    if (!proc_->isStarted()) {
        QString newmissing;
        QString mismatched;
        QString matched;
        QString sep;

        if (numbers.contains(FileStatus::FlagNewLost))
            newmissing = "* ";

        if (numbers.contains(FileStatus::Mismatched))
            mismatched = QString("☒%1").arg(numbers.numberOf(FileStatus::Mismatched));
        if (numbers.contains(FileStatus::Matched))
            matched = QString(" ✓%1").arg(numbers.numberOf(FileStatus::FlagMatched));

        if (numbers.contains(FileStatus::FlagChecked))
            sep = " : ";

        checkStatus = QString("%1%2%3%4").arg(newmissing, mismatched, matched, sep);

        // update only if the process is not running (there are no files with the "queued" status)
        availNumber = QString("%1 avail.").arg(numbers.numberOf(FileStatus::FlagAvailable));
        availSize = format::dataSizeReadable(numbers.totalSize(FileStatus::FlagAvailable));
    }

    return QString("%1%2 | %3 | %4")
                    .arg(checkStatus, // %1
                         availNumber, // %2
                         availSize, // %3
                         format::algoToStr(ui->treeView->data_->metaData.algorithm)); // %4
}

void MainWindow::handlePermanentStatusClick()
{
    if (proc_->isStarted())
        return;

    if (ui->treeView->isViewFileSystem() && !settings_->filter.extensionsList.isEmpty())
        dialogSettings();
    else if (ui->treeView->isViewDatabase())
        showDbStatus();
}

void MainWindow::handlePathEdit()
{
    (ui->pathEdit->text() == ui->treeView->curPathFileSystem) ? modeSelect->quickAction()
                                                              : ui->treeView->setIndexByPath(ui->pathEdit->text().replace("\\", "/"));
}

void MainWindow::handleChangedModel()
{
    bool isViewFS = ui->treeView->isViewFileSystem();

    ui->pathEdit->setEnabled(isViewFS);
    ui->pathEdit->setClearButtonEnabled(isViewFS);
    modeSelect->menuAct_->actionShowFilesystem->setEnabled(!isViewFS);
}

void MainWindow::createContextMenu_Button(const QPoint &point)
{
    if (modeSelect->isMode(Mode::File | Mode::Folder))
        modeSelect->menuAct_->menuAlgorithm(settings_->algorithm())->exec(ui->button->mapToGlobal(point));
}

bool MainWindow::argumentInput()
{
    if (QApplication::arguments().size() > 1) {
        QString argPath = QApplication::arguments().at(1);
        argPath.replace("\\", "/"); // win-->posix
        if (QFileInfo::exists(argPath)) {
            if (QFileInfo(argPath).isFile() && tools::isDatabaseFile(argPath)) {
                emit modeSelect->parseJsonFile(argPath);
            }
            else {
                ui->treeView->setFileSystemModel();
                ui->treeView->setIndexByPath(argPath);
            }
            return true;
        }
    }

    return false;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QString path = event->mimeData()->urls().first().toLocalFile();

    if (QFileInfo::exists(path)) {
        if (tools::isDatabaseFile(path)) {
            modeSelect->openJsonDatabase(path);
        }
        else {
            modeSelect->openFsPath(path);
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        if (modeSelect->processAbortPrompt() && !ui->treeView->isViewFileSystem()) {
            if (ui->treeView->isViewFiltered())
                ui->treeView->disableFilter();
            else
                modeSelect->showFileSystem(); // ui->treeView->setFileSystemModel();
        }
    }

    //QMainWindow::keyPressEvent(event);
}
