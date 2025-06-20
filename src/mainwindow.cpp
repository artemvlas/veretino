/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tools.h"
#include "pathstr.h"
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
    modeSelect->setProcState(manager->m_proc);
    ui->progressBar->setProcState(manager->m_proc);
    proc_ = manager->m_proc;

    modeSelect->m_menuAct->populateMenuFile(ui->menuFile);
    ui->menuHelp->addAction(modeSelect->m_menuAct->actionAbout);

    statusBar->setIconProvider(&modeSelect->m_icons);
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
        if (!awaiting_closure && manager->m_dataMaintainer->isDataNotSaved()) {
            awaiting_closure = true;
            modeSelect->saveData();
            event->ignore();
            return;
        }

        saveSettings();

        if (ui->view->isViewDatabase())
            ui->view->setModel(nullptr);

        event->accept();
    } else {
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
    connect(modeSelect->m_menuAct->actionOpenDialogSettings, &QAction::triggered, this, &MainWindow::dialogSettings);
    connect(modeSelect->m_menuAct->actionChooseFolder, &QAction::triggered, this, &MainWindow::dialogChooseFolder);
    connect(modeSelect->m_menuAct->actionOpenDatabaseFile, &QAction::triggered, this, &MainWindow::dialogOpenJson);
    connect(modeSelect->m_menuAct->actionAbout, &QAction::triggered, this, [=]{ DialogAbout about(this); about.exec(); });
    connect(ui->menuFile, &QMenu::aboutToShow, modeSelect->m_menuAct, qOverload<>(&MenuActions::updateMenuOpenRecent));

    // statusbar
    connect(statusBar, &StatusBar::buttonFsFilterClicked, this, &MainWindow::dialogSettings);
    connect(statusBar, &StatusBar::buttonDbListedClicked, this, [=]{ showDbStatusTab(DialogDbStatus::TabListed); });
    connect(statusBar, &StatusBar::buttonDbContentsClicked, modeSelect, &ModeSelector::makeDbContList);
    connect(statusBar, &StatusBar::buttonDbHashClicked, this, &MainWindow::handleButtonDbHashClick);
    connect(manager->m_proc, &ProcState::progressStarted, this,
            [=] { if (modeSelect->isMode(Mode::DbProcessing)) statusBar->setButtonsEnabled(false); });
    connect(manager->m_proc, &ProcState::progressFinished, this,
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
    qRegisterMetaType<DbFileState>("DbFileState");

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
    connect(manager->m_dataMaintainer, &DataMaintainer::databaseUpdated, this, &MainWindow::showDbStatus);
    connect(manager->m_dataMaintainer, &DataMaintainer::numbersUpdated, this, &MainWindow::updatePermanentStatus);
    connect(manager->m_dataMaintainer, &DataMaintainer::numbersUpdated, this, &MainWindow::updateWindowTitle);
    connect(manager->m_dataMaintainer, &DataMaintainer::subDbForked, this, &MainWindow::promptOpenBranch);
    connect(manager->m_dataMaintainer, &DataMaintainer::failedDataCreation, this,
            [=]{ if (ui->view->isViewModel(ModelView::NotSetted)) modeSelect->showFileSystem(); });
    connect(manager->m_dataMaintainer, &DataMaintainer::failedJsonSave, this, &MainWindow::dialogSaveJson);
    connect(manager->m_dataMaintainer, &DataMaintainer::dbFileStateChanged, this, &MainWindow::updateMenuActions);
    connect(manager->m_dataMaintainer, &DataMaintainer::dbFileStateChanged, this, [=]{ if (awaiting_closure) close(); });

    // process status
    connect(manager->m_proc, &ProcState::stateChanged, this, [=]{ if (proc_->isState(State::Idle)) ui->view->setViewProxy(); });
    connect(manager->m_proc, &ProcState::stateChanged, this, &MainWindow::updateButtonInfo);
    connect(manager->m_proc, &ProcState::progressStarted, ui->progressBar, &ProgressBar::start);
    connect(manager->m_proc, &ProcState::progressFinished, ui->progressBar, &ProgressBar::finish);
    connect(manager->m_proc, &ProcState::percentageChanged, ui->progressBar, &ProgressBar::setValue);
    connect(manager->m_proc, &ProcState::stateChanged, this, &MainWindow::updateStatusIcon);

    // <!> experimental
    connect(manager->m_proc, &ProcState::progressFinished, modeSelect, &ModeSelector::getInfoPathItem);

    // change view
    connect(manager, &Manager::switchToFsPrepared, [=] { ui->view->setFileSystemModel(); proc_->setAwaiting(ProcState::AwaitingNothing); });
    connect(ui->view, &View::switchedToFs, manager->m_dataMaintainer, &DataMaintainer::clearData);
    connect(ui->view, &View::modelChanged, manager, &Manager::modelChanged);
    connect(ui->view, &View::dataSetted, manager->m_dataMaintainer, &DataMaintainer::clearOldData);
    connect(ui->view, &View::dataSetted, this,
            [=]{ if (ui->view->m_data) settings_->addRecentFile(ui->view->m_data->m_metadata.dbFilePath); });

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
        DialogDbStatus statusDialog(ui->view->m_data, this);
        statusDialog.setCurrentTab(tab);

        statusDialog.exec();
    }
}

void MainWindow::showDialogContentsList(const QString &folderName, const FileTypeList &extList)
{
    if (!extList.isEmpty()) {
        DialogContentsList dialog(folderName, extList, this);
        dialog.setWindowIcon(modeSelect->m_icons.iconFolder());
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
        DialogExistingDbs dlg(dbFiles, this);
        if (dlg.exec() && !dlg.curFile().isEmpty()) {
            modeSelect->openJsonDatabase(pathstr::joinPath(folder, dlg.curFile()));
            return;
        }
    }

    DialogDbCreation dialog(folder, extList, this);
    dialog.setSettings(settings_);
    dialog.setWindowTitle(QStringLiteral(u"Creating a new database..."));

    if (!dialog.exec()) {
        return;
    }

    const FilterRule filterRule = dialog.resultFilter();
    if (filterRule || !dialog.isFilterCreating()) {
        modeSelect->processFolderChecksums(filterRule);
    }
    else { // filter creation is enabled, BUT no suffix(type) is ​​selected
        QMessageBox msgBox(this);
        msgBox.setIconPixmap(modeSelect->m_icons.pixmap(Icons::Filter));
        msgBox.setWindowTitle("No filter specified");
        msgBox.setText("File filtering is not set.");
        msgBox.setInformativeText("Continue with all files?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Abort);
        msgBox.setDefaultButton(QMessageBox::Yes);
        msgBox.button(QMessageBox::Yes)->setText("Continue");

        if (msgBox.exec() == QMessageBox::Yes)
            modeSelect->processFolderChecksums(filterRule);
    }
}

void MainWindow::showDialogDbContents(const QString &folderName, const FileTypeList &extList)
{
    if (!extList.isEmpty()) {
        DialogContentsList dialog(folderName, extList, this);
        dialog.setWindowIcon(modeSelect->m_icons.icon(Icons::Database));
        QString strWindowTitle = QStringLiteral(u"Database Contents");
        if (ui->view->m_data && ui->view->m_data->m_numbers.contains(FileStatus::Missing))
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
        } else {
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.button(QMessageBox::Ok)->setText("Update");
            msgBox.button(QMessageBox::Ok)->setIcon(modeSelect->m_icons.icon(FileStatus::Updated));
            msgBox.setDefaultButton(QMessageBox::Cancel);
        }

        msgBox.button(QMessageBox::Cancel)->setText("Continue");
        msgBox.button(QMessageBox::Cancel)->setIcon(QIcon());

        const int n_mismatch = result.numberOf(FileStatus::Mismatched);
        messageText.append(QString("%1 out of %2 files %3 changed or corrupted.")
                            .arg(n_mismatch)
                            .arg(result.numberOf(FileStatus::CombMatched | FileStatus::Mismatched)) // was before: FileStatus::CombAvailable
                            .arg(n_mismatch == 1 ? "is" : "are"));

        const int n_notchecked = result.numberOf(FileStatus::CombNotChecked);
        if (n_notchecked) {
            messageText.append("\n\n");
            messageText.append(format::filesNumber(n_notchecked)
                               + " remain unchecked.");
        }
    } else {
        messageText.append(QString("ALL %1 files passed verification.")
                            .arg(result.numberOf(FileStatus::CombMatched)));
    }

    FileStatus st_icon = result.contains(FileStatus::Mismatched) ? FileStatus::Mismatched
                                                                 : FileStatus::Matched;

    msgBox.setIconPixmap(modeSelect->m_icons.pixmap(st_icon));
    msgBox.setWindowTitle(titleText);
    msgBox.setText(messageText);

    const int ret = msgBox.exec();

    if (!modeSelect->isDbConst()
        && result.contains(FileStatus::Mismatched)
        && ret == QMessageBox::Ok)
    {
        modeSelect->updateDatabase(DbMod::DM_UpdateMismatches);
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
            DataContainer *dc = ui->view->m_data;

            if (settings_->detectMoved
                && dc->_cacheMissing.isEmpty()
                && dc->hasPossiblyMovedItems())
            {
                manager->addTaskWithState(State::Idle, &Manager::cacheMissingItems);
            }
            else if (!settings_->detectMoved
                       && !dc->_cacheMissing.isEmpty())
            {
                dc->_cacheMissing.clear();
            }
        }
    }
}

void MainWindow::dialogChooseFolder()
{
    const QString path = QFileDialog::getExistingDirectory(this, QString(), QDir::homePath());

    if (!path.isEmpty()) {
        modeSelect->showFileSystem(path);
    }
}

void MainWindow::dialogOpenJson()
{
    const QString path = QFileDialog::getOpenFileName(this, "Select Veretino Database",
                                                      QDir::homePath(),
                                                      "Veretino DB (*.ver *.ver.json)");

    if (!path.isEmpty()) {
        modeSelect->openJsonDatabase(path);
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
    msgBox.setIconPixmap(modeSelect->m_icons.pixmap(Icons::AddFork));
    msgBox.setWindowTitle("A new Branch has been created");
    msgBox.setText("The subfolder data is forked:\n" + pathstr::shortenPath(dbFilePath));
    msgBox.setInformativeText("Do you want to open it or stay in the current one?");
    msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::No)->setText("Stay");

    if (msgBox.exec() == QMessageBox::Open) {
        modeSelect->openJsonDatabase(dbFilePath);
    }
}

void MainWindow::dialogSaveJson(VerJson *pUnsavedJson)
{
    if (!pUnsavedJson) {
        qWarning() << "No unsaved json found";
        return;
    }

    QString file_path = pUnsavedJson->file_path();

    const bool is_short = pathstr::hasExtension(file_path, Lit::sl_db_exts.at(1));
    const QString ext = Lit::sl_db_exts.at(is_short); // index 0 is long, 1 is short
    const QString str_filter = QStringLiteral(u"Veretino DB (*.%1)").arg(ext);

    while (true) {
        const QString sel_path = QFileDialog::getSaveFileName(this,
                                                              QStringLiteral(u"Save DB File"),
                                                              file_path,
                                                              str_filter);

        if (sel_path.isEmpty()) { // canceled
            // TODO: should be merged different 'awaiting' methods
            if ((awaiting_closure || proc_->isAwaiting(ProcState::AwaitingSwitchToFs))
                && QMessageBox::question(this, "Unsaved data...",
                                         "Close without saving data?") != QMessageBox::Yes)
            {
                continue;
            }

            break;
        }

        file_path = sel_path;

        if (!pathstr::hasExtension(file_path, ext))
            file_path = pathstr::setSuffix(file_path, ext);

        pUnsavedJson->setFilePath(file_path);

        if (manager->m_dataMaintainer->saveJsonFile(pUnsavedJson))
            break;
    }

    delete pUnsavedJson;
    qDebug() << "dialogSaveJson:" << file_path;

    // TODO: should be merged different 'awaiting' methods
    if (awaiting_closure) {
        close();
    } else if (proc_->isAwaiting(ProcState::AwaitingSwitchToFs)) {
        ui->view->setFileSystemModel();
        proc_->setAwaiting(ProcState::AwaitingNothing);
    }
}

void MainWindow::updateButtonInfo()
{
    ui->button->setIcon(modeSelect->getButtonIcon());
    ui->button->setText(modeSelect->getButtonText());
    ui->button->setToolTip(modeSelect->getButtonToolTip());
}

void MainWindow::updateMenuActions()
{
    modeSelect->m_menuAct->actionShowFilesystem->setEnabled(!ui->view->isViewFileSystem());
    modeSelect->m_menuAct->actionSave->setEnabled(ui->view->isViewDatabase()
                                                  && ui->view->m_data->isDbFileState(DbFileState::NotSaved));
}

void MainWindow::updateStatusIcon()
{
    QIcon statusIcon;
    if (proc_->isStarted()) {
        statusIcon = modeSelect->m_icons.icon(FileStatus::Calculating);
    }
    else if (ui->view->isViewFileSystem()) {
        statusIcon = modeSelect->m_icons.icon(ui->view->curAbsPath());
    }
    else if (ui->view->isViewDatabase()) {
        if (TreeModel::isFileRow(ui->view->curIndex()))
            statusIcon = modeSelect->m_icons.icon(TreeModel::itemFileStatus(ui->view->curIndex()));
        else
            statusIcon = modeSelect->m_icons.iconFolder();
    }

    statusBar->setStatusIcon(statusIcon);
}

void MainWindow::updatePermanentStatus()
{
    if (ui->view->isViewDatabase()) {
        if (modeSelect->isMode(Mode::DbCreating))
            statusBar->setModeDbCreating();
        else if (!proc_->isStarted())
            statusBar->setModeDb(ui->view->m_data);
    } else {
        statusBar->clearButtons();
    }
}

void MainWindow::setWinTitleMismatchFound()
{
    static const QString str = tools::joinStrings(Lit::s_appName,
                                                  QStringLiteral(u"<!> mismatches found"),
                                                  Lit::s_sepStick);
    setWindowTitle(str);
}

void MainWindow::updateWindowTitle()
{
    if (ui->view->isViewDatabase()) {
        const DataContainer *data = ui->view->m_data;

        if (data->m_numbers.contains(FileStatus::Mismatched)) {
            setWinTitleMismatchFound();
            return;
        }

        QString strAdd = data->isAllMatched() ? QStringLiteral(u"✓ verified")
                                              : QStringLiteral(u"DB > ") + pathstr::shortenPath(data->m_metadata.workDir);

        const QString strTitle = tools::joinStrings(Lit::s_appName, strAdd, Lit::s_sepStick);
        setWindowTitle(strTitle);
    } else {
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

    updateMenuActions();
}

void MainWindow::handleButtonDbHashClick()
{
    if (!proc_->isStarted() && ui->view->isViewDatabase()) {
        const Numbers &nums = ui->view->m_data->m_numbers;
        if (nums.contains(FileStatus::CombChecked))
            showDbStatusTab(DialogDbStatus::TabVerification);
        else if (nums.contains(FileStatus::CombDbChanged))
            showDbStatusTab(DialogDbStatus::TabChanges);
        else
            showMessage("There are no checked items yet.", "Unchecked DB");
    }
}

void MainWindow::createContextMenu_Button(const QPoint &point)
{
    if (modeSelect->isMode(Mode::File | Mode::Folder)) {
        modeSelect->m_menuAct->menuAlgorithm(settings_->algorithm())->exec(ui->button->mapToGlobal(point));
    }
}

void MainWindow::clearDialogs()
{
    QDialog *dlg = findChild<QDialog*>();

    if (dlg)
        dlg->reject();
}

bool MainWindow::argumentInput()
{
    if (QApplication::arguments().size() > 1) {
        QString argPath = QApplication::arguments().at(1);
        argPath.replace('\\', '/'); // win-->posix
        if (QFileInfo::exists(argPath)) {
            if (QFileInfo(argPath).isFile() && paths::isDbFile(argPath)) {
                modeSelect->openJsonDatabase(argPath);
            } else {
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
    const QString path = event->mimeData()->urls().first().toLocalFile();

    if (QFileInfo::exists(path)) {
        if (paths::isDbFile(path)) {
            modeSelect->openJsonDatabase(path);
        } else {
            modeSelect->showFileSystem(path);
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
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
