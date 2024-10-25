/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tools.h"
#include "dialogcontentslist.h"
#include "dialogdbcreation.h"
#include "dialogexistingdbs.h"
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
    QThread::currentThread()->setObjectName("Main Thread");
    thread->setObjectName("Manager Thread");

    ui->button->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->view->setSettings(settings_);

    settings_->loadSettings();

    restoreGeometry(settings_->geometryMainWindow);

    modeSelect = new ModeSelector(ui->view, settings_, this);
    modeSelect->setManager(manager);
    modeSelect->setProcState(manager->procState);
    ui->progressBar->setProcState(manager->procState);
    proc_ = manager->procState;

    modeSelect->menuAct_->populateMenuFile(ui->menuFile);
    ui->menuHelp->addAction(modeSelect->menuAct_->actionAbout);

    statusBar->setIconProvider(&modeSelect->_icons);
    setStatusBar(statusBar);
    updatePermanentStatus();

    connections();

    if (!argumentInput())
        ui->view->setFileSystemModel();
}

MainWindow::~MainWindow()
{
    thread->quit();
    thread->wait();

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // if a computing process is running, show a prompt when user wants to close the app
    if (modeSelect->promptProcessAbort()) {
        modeSelect->saveData();
        saveSettings();

        if (ui->view->isViewDatabase())
            ui->view->setModel(nullptr);

        event->accept();
    }
    else {
        event->ignore();
    }
}

void MainWindow::connections()
{
    connectManager();

    // Push Button
    connect(ui->button, &QPushButton::clicked, modeSelect, &ModeSelector::doWork);
    connect(ui->button, &QPushButton::customContextMenuRequested, this, &MainWindow::createContextMenu_Button);

    connect(settings_, &Settings::algorithmChanged, this, &MainWindow::updateButtonInfo);

    // TreeView
    connect(ui->view, &View::keyEnterPressed, modeSelect, &ModeSelector::quickAction);
    connect(ui->view, &View::doubleClicked, modeSelect, &ModeSelector::quickAction);
    connect(ui->view, &View::customContextMenuRequested, modeSelect, &ModeSelector::createContextMenu_View);
    connect(ui->view, &View::pathChanged, ui->pathEdit, &QLineEdit::setText);
    connect(ui->view, &View::pathChanged, modeSelect, &ModeSelector::getInfoPathItem);
    connect(ui->view, &View::pathChanged, this, &MainWindow::updateButtonInfo);
    connect(ui->view, &View::pathChanged, this, &MainWindow::updateStatusIcon);
    connect(ui->view, &View::modelChanged, this, &MainWindow::handleChangedModel);
    connect(ui->view, &View::modelChanged, this, &MainWindow::updatePermanentStatus);
    connect(ui->view, &View::modelChanged, this, &MainWindow::updateWindowTitle);
    connect(ui->view, &View::showMessage, this, &MainWindow::showMessage);
    connect(ui->view, &View::showDbStatus, this, &MainWindow::showDbStatus);

    connect(ui->pathEdit, &QLineEdit::returnPressed, this, &MainWindow::handlePathEdit);

    // menu actions
    connect(modeSelect->menuAct_->actionOpenDialogSettings, &QAction::triggered, this, &MainWindow::dialogSettings);
    connect(modeSelect->menuAct_->actionChooseFolder, &QAction::triggered, this, &MainWindow::dialogChooseFolder);
    connect(modeSelect->menuAct_->actionOpenDatabaseFile, &QAction::triggered, this, &MainWindow::dialogOpenJson);
    connect(modeSelect->menuAct_->actionAbout, &QAction::triggered, this, [=]{ DialogAbout about(this); about.exec(); });
    connect(ui->menuFile, &QMenu::aboutToShow, modeSelect->menuAct_, qOverload<>(&MenuActions::updateMenuOpenRecent));
    connect(manager->dataMaintainer, &DataMaintainer::dbFileStateChanged, modeSelect->menuAct_->actionSave, &QAction::setEnabled);

    // statusbar
    connect(statusBar, &StatusBar::buttonFsFilterClicked, this, &MainWindow::dialogSettings);
    connect(statusBar, &StatusBar::buttonDbListedClicked, this, [=]{ showDbStatusTab(DialogDbStatus::TabListed); });
    connect(statusBar, &StatusBar::buttonDbContentsClicked, modeSelect, &ModeSelector::_makeDbContentsList);
    connect(statusBar, &StatusBar::buttonDbHashClicked, this, &MainWindow::handleButtonDbHashClick);
    connect(manager->procState, &ProcState::progressStarted, this,
            [=] { if (modeSelect->isMode(Mode::DbProcessing)) statusBar->setButtonsEnabled(false); });
    connect(manager->procState, &ProcState::progressFinished, this,
            [=] { if (ui->view->isViewDatabase()) statusBar->setButtonsEnabled(true); });
}

void MainWindow::connectManager()
{
    qRegisterMetaType<QVector<int>>("QVector<int>"); // for building on Windows (qt 5.15.2)
    qRegisterMetaType<QCryptographicHash::Algorithm>("QCryptographicHash::Algorithm");
    qRegisterMetaType<ModelView>("ModelView");
    qRegisterMetaType<FileTypeList>("FileTypeList");
    qRegisterMetaType<Numbers>("Numbers");
    qRegisterMetaType<FileValues>("FileValues");

    manager->moveToThread(thread);

    // when closing the thread
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(thread, &QThread::finished, manager, &Manager::deleteLater);

    // info and notifications
    connect(manager, &Manager::setStatusbarText, statusBar, &StatusBar::setStatusText);
    connect(manager, &Manager::showMessage, this, &MainWindow::showMessage);
    connect(manager, &Manager::folderChecked, ui->view, &View::setMismatchFiltering);
    connect(manager, &Manager::folderChecked, this, &MainWindow::showFolderCheckResult);
    connect(manager, &Manager::fileProcessed, this, &MainWindow::showFileCheckResult);
    connect(manager, &Manager::folderContentsListCreated, this, &MainWindow::showDialogContentsList);
    connect(manager, &Manager::dbCreationDataCollected, this, &MainWindow::showDialogDbCreation);
    connect(manager, &Manager::dbContentsListCreated, this, &MainWindow::showDialogDbContents);
    connect(manager, &Manager::mismatchFound, this, &MainWindow::setWinTitleMismatchFound);

    // results processing
    connect(manager, &Manager::setViewData, ui->view, &View::setData);
    connect(manager->dataMaintainer, &DataMaintainer::databaseUpdated, this, &MainWindow::showDbStatus);
    connect(manager->dataMaintainer, &DataMaintainer::numbersUpdated, this, &MainWindow::updatePermanentStatus);
    connect(manager->dataMaintainer, &DataMaintainer::numbersUpdated, this, &MainWindow::updateWindowTitle);
    connect(manager->dataMaintainer, &DataMaintainer::subDbForked, this, &MainWindow::promptOpenBranch);
    connect(manager->dataMaintainer, &DataMaintainer::failedDataCreation, this,
            [=]{ if (ui->view->isViewModel(ModelView::NotSetted)) modeSelect->showFileSystem(); });

    // process status
    connect(manager->procState, &ProcState::stateChanged, this, [=]{ if (proc_->isState(State::Idle)) ui->view->setViewProxy(); });
    connect(manager->procState, &ProcState::stateChanged, this, &MainWindow::updateButtonInfo);
    connect(manager->procState, &ProcState::progressStarted, ui->progressBar, &ProgressBar::start);
    connect(manager->procState, &ProcState::progressFinished, ui->progressBar, &ProgressBar::finish);
    connect(manager->procState, &ProcState::percentageChanged, ui->progressBar, &ProgressBar::setValue);
    connect(manager->procState, &ProcState::stateChanged, this, &MainWindow::updateStatusIcon);

    // <!> experimental
    connect(manager->procState, &ProcState::progressFinished, modeSelect, &ModeSelector::getInfoPathItem);

    // change view
    connect(manager, &Manager::switchToFsPrepared, ui->view, &View::setFileSystemModel);
    connect(ui->view, &View::switchedToFs, manager->dataMaintainer, &DataMaintainer::clearData);
    connect(ui->view, &View::modelChanged, manager, &Manager::modelChanged);
    connect(ui->view, &View::dataSetted, manager->dataMaintainer, &DataMaintainer::clearOldData);
    connect(ui->view, &View::dataSetted, this,
            [=]{ if (ui->view->data_) settings_->addRecentFile(ui->view->data_->metaData_.dbFilePath); });

    thread->start();
}

void MainWindow::saveSettings()
{
    ui->view->saveHeaderState();
    settings_->geometryMainWindow = saveGeometry();
    settings_->saveSettings();
}

void MainWindow::showDbStatus()
{
    showDbStatusTab(DialogDbStatus::TabAutoSelect);
}

void MainWindow::showDbStatusTab(DialogDbStatus::Tabs tab)
{
    if (proc_->isState(State::StartSilently)) {
        qDebug() << "MainWindow::showDbStatusTab | Rejected >> State::StartSilently";
        return;
    }

    clearDialogs();

    if (modeSelect->isMode(Mode::DbIdle | Mode::DbCreating)) {
        DialogDbStatus statusDialog(ui->view->data_, this);
        statusDialog.setCurrentTab(tab);

        /*if (proc_->isStarted()) { // replaced with ::clearDialogs()
            connect(proc_, &ProcState::progressFinished, &statusDialog, &DialogDbStatus::reject); // TMP
        }*/

        statusDialog.exec();
    }
}

void MainWindow::showDialogContentsList(const QString &folderName, const FileTypeList &extList)
{
    if (!extList.isEmpty()) {
        DialogContentsList dialog(folderName, extList, this);
        dialog.setWindowIcon(modeSelect->_icons.iconFolder());
        dialog.exec();
    }
}

void MainWindow::showDialogDbCreation(const QString &folder, const QStringList &dbFiles, const FileTypeList &extList)
{
    if (extList.isEmpty()) {
        return;
    }

    clearDialogs();

    if (!dbFiles.isEmpty()) {
        DialogExistingDbs _dialog(dbFiles, this);
        if (_dialog.exec() && !_dialog.curFile().isEmpty()) {
            modeSelect->openJsonDatabase(paths::joinPath(folder, _dialog.curFile()));
            return;
        }
    }

    DialogDbCreation dialog(folder, extList, this);
    dialog.setSettings(settings_);
    dialog.setWindowTitle(QStringLiteral(u"Creating a new database..."));

    if (!dialog.exec()) {
        return;
    }

    const FilterRule _filter = dialog.resultFilter();
    if (_filter || !dialog.isFilterCreating()) {
        modeSelect->processFolderChecksums(_filter);
    }
    else { // filter creation is enabled, BUT no suffix(type) is ​​selected
        QMessageBox msgBox(this);
        msgBox.setIconPixmap(modeSelect->_icons.pixmap(Icons::Filter));
        msgBox.setWindowTitle("No filter specified");
        msgBox.setText("File filtering is not set.");
        msgBox.setInformativeText("Continue with all files?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Abort);
        msgBox.setDefaultButton(QMessageBox::Yes);
        msgBox.button(QMessageBox::Yes)->setText("Continue");

        if (msgBox.exec() == QMessageBox::Yes)
            modeSelect->processFolderChecksums(_filter);
    }
}

void MainWindow::showDialogDbContents(const QString &folderName, const FileTypeList &extList)
{
    if (!extList.isEmpty()) {
        DialogContentsList dialog(folderName, extList, this);
        dialog.setWindowIcon(modeSelect->_icons.icon(Icons::Database));
        QString strWindowTitle = QStringLiteral(u"Database Contents");
        if (ui->view->data_ && ui->view->data_->numbers_.contains(FileStatus::Missing))
            strWindowTitle.append(QStringLiteral(u" [available ones]"));
        dialog.setWindowTitle(strWindowTitle);
        dialog.exec();
    }
}

void MainWindow::showFolderCheckResult(const Numbers &result, const QString &subFolder)
{
    clearDialogs();

    QMessageBox msgBox(this);

    QString titleText = result.contains(FileStatus::Mismatched) ? "FAILED" : "Success";
    QString messageText = !subFolder.isEmpty() ? QString("Subfolder: %1\n\n").arg(subFolder) : QString();

    if (result.contains(FileStatus::Mismatched)) {
        if (modeSelect->isDbConst()) {
            msgBox.setStandardButtons(QMessageBox::Cancel);
        }
        else {
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.button(QMessageBox::Ok)->setText("Update");
            msgBox.button(QMessageBox::Ok)->setIcon(modeSelect->_icons.icon(FileStatus::Updated));
            msgBox.setDefaultButton(QMessageBox::Cancel);
        }

        msgBox.button(QMessageBox::Cancel)->setText("Continue");
        msgBox.button(QMessageBox::Cancel)->setIcon(QIcon());

        const int _n_mismatch = result.numberOf(FileStatus::Mismatched);
        messageText.append(QString("%1 out of %2 files %3 changed or corrupted.")
                            .arg(_n_mismatch)
                            .arg(result.numberOf(FileStatus::CombMatched | FileStatus::Mismatched)) // was before: FileStatus::CombAvailable
                            .arg(_n_mismatch == 1 ? "is" : "are"));

        const int _n_notchecked = result.numberOf(FileStatus::CombNotChecked);
        if (_n_notchecked) {
            messageText.append("\n\n");
            messageText.append(format::filesNumber(_n_notchecked)
                               + " remain unchecked.");
        }
    }
    else {
        messageText.append(QString("ALL %1 files passed verification.")
                            .arg(result.numberOf(FileStatus::CombMatched)));
    }

    FileStatus _st_icon = result.contains(FileStatus::Mismatched) ? FileStatus::Mismatched
                                                                  : FileStatus::Matched;

    msgBox.setIconPixmap(modeSelect->_icons.pixmap(_st_icon));
    msgBox.setWindowTitle(titleText);
    msgBox.setText(messageText);

    int ret = msgBox.exec();

    if (!modeSelect->isDbConst()
        && result.contains(FileStatus::Mismatched)
        && ret == QMessageBox::Ok)
    {
        modeSelect->updateDatabase(DestDbUpdate::DestUpdateMismatches);
    }
}

void MainWindow::showFileCheckResult(const QString &filePath, const FileValues &values)
{
    clearDialogs();

    DialogFileProcResult dialog(filePath, values, this);
    dialog.exec();
}

void MainWindow::dialogSettings()
{
    DialogSettings dialog(settings_, this);

    if (dialog.exec()) {
        dialog.updateSettings();

        // switching "Detect Moved" cache
        if (ui->view->isViewDatabase()) {
            DataContainer *_d = ui->view->data_;

            if (settings_->detectMoved
                && _d->_cacheMissing.isEmpty()
                && _d->hasPossiblyMovedItems())
            {
                manager->addTaskWithState(State::Idle, &Manager::cacheMissingItems);
            }
            else if (!settings_->detectMoved
                       && !_d->_cacheMissing.isEmpty())
            {
                _d->_cacheMissing.clear();
            }
        }
    }
}

void MainWindow::dialogChooseFolder()
{
    const QString _path = QFileDialog::getExistingDirectory(this, QString(), QDir::homePath());

    if (!_path.isEmpty()) {
        modeSelect->showFileSystem(_path);
    }
}

void MainWindow::dialogOpenJson()
{
    const QString _path = QFileDialog::getOpenFileName(this, "Select Veretino Database",
                                                       QDir::homePath(),
                                                       "Veretino DB (*.ver *.ver.json)");

    if (!_path.isEmpty()) {
        modeSelect->openJsonDatabase(_path);
    }
}

void MainWindow::showMessage(const QString &message, const QString &title)
{
    clearDialogs();

    QMessageBox messageBox;

    if (!title.compare(QStringLiteral(u"error"), Qt::CaseInsensitive)
        || !title.compare(QStringLiteral(u"failed"), Qt::CaseInsensitive))
    {
        messageBox.critical(this, title, message);
    }
    else if (!title.compare(QStringLiteral(u"warning"), Qt::CaseInsensitive)) {
        messageBox.warning(this, title, message);
    }
    else {
        messageBox.information(this, title, message);
    }
}

void MainWindow::promptOpenBranch(const QString &dbFilePath)
{
    if (!paths::isDbFile(dbFilePath))
        return;

    QMessageBox msgBox(this);
    msgBox.setIconPixmap(modeSelect->_icons.pixmap(Icons::AddFork));
    msgBox.setWindowTitle("A new Branch has been created");
    msgBox.setText("The subfolder data is forked:\n" + paths::shortenPath(dbFilePath));
    msgBox.setInformativeText("Do you want to open it or stay in the current one?");
    msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
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
        statusIcon = modeSelect->_icons.icon(FileStatus::Calculating);
    }
    else if (ui->view->isViewFileSystem()) {
        statusIcon = modeSelect->_icons.icon(ui->view->curAbsPath());
    }
    else if (ui->view->isViewDatabase()) {
        if (TreeModel::isFileRow(ui->view->curIndex()))
            statusIcon = modeSelect->_icons.icon(TreeModel::itemFileStatus(ui->view->curIndex()));
        else
            statusIcon = modeSelect->_icons.iconFolder();
    }

    statusBar->setStatusIcon(statusIcon);
}

void MainWindow::updatePermanentStatus()
{
    if (ui->view->isViewDatabase()) {
        if (modeSelect->isMode(Mode::DbCreating))
            statusBar->setModeDbCreating();
        else if (!proc_->isStarted())
            statusBar->setModeDb(ui->view->data_);
    }
    else {
        statusBar->clearButtons();
    }
}

void MainWindow::setWinTitleMismatchFound()
{
    static const QString _s = tools::joinStrings(Lit::s_appName,
                                                 QStringLiteral(u"<!> mismatches found"),
                                                 Lit::s_sepStick);
    setWindowTitle(_s);
}

void MainWindow::updateWindowTitle()
{
    if (ui->view->isViewDatabase()) {
        const DataContainer *data = ui->view->data_;

        if (data->numbers_.contains(FileStatus::Mismatched)) {
            setWinTitleMismatchFound();
            return;
        }

        QString _s_add = data->isAllMatched() ? QStringLiteral(u"✓ verified")
                                              : QStringLiteral(u"DB > ") + paths::shortenPath(data->metaData_.workDir);

        const QString _s = tools::joinStrings(Lit::s_appName, _s_add, Lit::s_sepStick);
        setWindowTitle(_s);
    }
    else {
        setWindowTitle(Lit::s_appName);
    }
}

void MainWindow::handlePathEdit()
{
    if (ui->pathEdit->text() == ui->view->curAbsPath())
        modeSelect->quickAction();
    else
        ui->view->setIndexByPath(ui->pathEdit->text().replace('\\', '/'));
}

void MainWindow::handleChangedModel()
{
    bool isViewFS = ui->view->isViewFileSystem();

    ui->pathEdit->setEnabled(isViewFS);
    ui->pathEdit->setClearButtonEnabled(isViewFS);

    modeSelect->menuAct_->actionShowFilesystem->setEnabled(!isViewFS);
    modeSelect->menuAct_->actionSave->setEnabled(ui->view->isViewDatabase()
                                                 && ui->view->data_->isDbFileState(DbFileState::NotSaved));
}

void MainWindow::handleButtonDbHashClick()
{
    if (!proc_->isStarted() && ui->view->isViewDatabase()) {
        Numbers &numbers = ui->view->data_->numbers_;
        if (numbers.contains(FileStatus::CombChecked))
            showDbStatusTab(DialogDbStatus::TabVerification);
        else if (numbers.contains(FileStatus::CombDbChanged))
            showDbStatusTab(DialogDbStatus::TabChanges);
        else
            showMessage("There are no checked items yet.", "Unchecked DB");
    }
}

void MainWindow::createContextMenu_Button(const QPoint &point)
{
    if (modeSelect->isMode(Mode::File | Mode::Folder)) {
        modeSelect->menuAct_->menuAlgorithm(settings_->algorithm())->exec(ui->button->mapToGlobal(point));
    }
}

void MainWindow::clearDialogs()
{
    QDialog *__d = findChild<QDialog*>();

    if (__d)
        __d->reject();
}

bool MainWindow::argumentInput()
{
    if (QApplication::arguments().size() > 1) {
        QString argPath = QApplication::arguments().at(1);
        argPath.replace('\\', '/'); // win-->posix
        if (QFileInfo::exists(argPath)) {
            if (QFileInfo(argPath).isFile() && paths::isDbFile(argPath)) {
                modeSelect->openJsonDatabase(argPath);
            }
            else {
                ui->view->setFileSystemModel();
                ui->view->setIndexByPath(argPath);
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
        if (paths::isDbFile(path)) {
            modeSelect->openJsonDatabase(path);
        }
        else {
            modeSelect->showFileSystem(path);
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        // Idle DB
        if (modeSelect->isMode(Mode::DbIdle)) {
            if (ui->view->isViewFiltered())
                ui->view->disableFilter();
            else if (QMessageBox::question(this, "Exit...", "Close the Database?") == QMessageBox::Yes)
                modeSelect->showFileSystem();
        }
        // Verifying/Updating (not creating) DB || filesystem process
        else if (modeSelect->isMode(Mode::DbProcessing | Mode::FileProcessing)) {
            modeSelect->promptProcessStop();
        }
        // other cases
        else if (modeSelect->promptProcessAbort()
                 && !ui->view->isViewFileSystem())
        {
            modeSelect->showFileSystem();
        }
    }
    // temporary solution until appropriate Actions are added
    else if (event->key() == Qt::Key_F1
             && modeSelect->isMode(Mode::DbIdle | Mode::DbCreating))
    {
        showDbStatus();
    }
    else if (event->key() == Qt::Key_F5
             && !proc_->isStarted() && modeSelect->isMode(Mode::DbIdle))
    {
        modeSelect->resetDatabase();
    }
    // TMP ^^^

    QMainWindow::keyPressEvent(event);
}
