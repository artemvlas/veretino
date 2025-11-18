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
    m_thread->setObjectName("Manager Thread");
    m_proc = m_manager->m_proc;

    ui->button->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->view->setSettings(m_settings);

    m_settings->loadSettings();

    restoreGeometry(m_settings->geometryMainWindow);

    m_modeSelect = new ModeSelector(ui->view, m_settings, this);
    m_modeSelect->setManager(m_manager);
    m_modeSelect->setProcState(m_proc);
    ui->progressBar->setProcState(m_proc);

    m_modeSelect->m_menuAct->populateMenuFile(ui->menuFile);
    ui->menuHelp->addAction(m_modeSelect->m_menuAct->actionAbout);

    m_statusBar->setIconProvider(&m_modeSelect->m_icons);
    setStatusBar(m_statusBar);
    updatePermanentStatus();

    connections();

    if (!argumentInput())
        ui->view->setFileSystemModel();
}

MainWindow::~MainWindow()
{
    m_thread->quit();
    m_thread->wait();

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // if a computing process is running, show a prompt when user wants to close the app
    if (m_modeSelect->promptProcessAbort()) {
        if (!m_proc->isAwaiting(ProcState::AwaitingClosure)
            && m_manager->m_dataMaintainer->isDataNotSaved())
        {
            m_proc->setAwaiting(ProcState::AwaitingClosure);
            m_modeSelect->saveData();
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
    connect(ui->button, &QPushButton::clicked, m_modeSelect, &ModeSelector::doWork);
    connect(ui->button, &QPushButton::customContextMenuRequested, this, &MainWindow::createContextMenu_Button);

    connect(m_settings, &Settings::algorithmChanged, this, &MainWindow::updateButtonInfo);

    // TreeView
    connect(ui->view, &View::keyEnterPressed, m_modeSelect, &ModeSelector::quickAction);
    connect(ui->view, &View::doubleClicked, m_modeSelect, &ModeSelector::quickAction);
    connect(ui->view, &View::customContextMenuRequested, m_modeSelect, &ModeSelector::createContextMenu_View);
    connect(ui->view, &View::pathChanged, ui->pathEdit, &QLineEdit::setText);
    connect(ui->view, &View::pathChanged, m_modeSelect, &ModeSelector::getInfoPathItem);
    connect(ui->view, &View::pathChanged, this, &MainWindow::updateButtonInfo);
    connect(ui->view, &View::pathChanged, this, &MainWindow::updateStatusIcon);
    connect(ui->view, &View::modelChanged, this, &MainWindow::handleChangedModel);
    connect(ui->view, &View::modelChanged, this, &MainWindow::updatePermanentStatus);
    connect(ui->view, &View::modelChanged, this, &MainWindow::updateWindowTitle);
    connect(ui->view, &View::showMessage, this, &MainWindow::showMessage);
    connect(ui->view, &View::showDbStatus, this, &MainWindow::showDbStatus);

    connect(ui->pathEdit, &QLineEdit::returnPressed, this, &MainWindow::handlePathEdit);

    // menu actions
    connect(m_modeSelect->m_menuAct->actionOpenDialogSettings, &QAction::triggered, this, &MainWindow::dialogSettings);
    connect(m_modeSelect->m_menuAct->actionChooseFolder, &QAction::triggered, this, &MainWindow::dialogChooseFolder);
    connect(m_modeSelect->m_menuAct->actionOpenDatabaseFile, &QAction::triggered, this, &MainWindow::dialogOpenJson);
    connect(m_modeSelect->m_menuAct->actionAbout, &QAction::triggered, this, [=]{ DialogAbout about(this); about.exec(); });
    connect(ui->menuFile, &QMenu::aboutToShow, m_modeSelect->m_menuAct, qOverload<>(&MenuActions::updateMenuOpenRecent));

    // statusbar
    connect(m_statusBar, &StatusBar::buttonFsFilterClicked, this, &MainWindow::dialogSettings);
    connect(m_statusBar, &StatusBar::buttonDbListedClicked, this, [=]{ showDbStatusTab(DialogDbStatus::TabListed); });
    connect(m_statusBar, &StatusBar::buttonDbContentsClicked, m_modeSelect, &ModeSelector::makeDbContList);
    connect(m_statusBar, &StatusBar::buttonDbHashClicked, this, &MainWindow::handleButtonDbHashClick);
    connect(m_proc, &ProcState::progressStarted, this,
            [=] { if (m_modeSelect->isMode(Mode::DbProcessing)) m_statusBar->setButtonsEnabled(false); });
    connect(m_proc, &ProcState::progressFinished, this,
            [=] { if (ui->view->isViewDatabase()) m_statusBar->setButtonsEnabled(true); });
}

void MainWindow::connectManager()
{
    qRegisterMetaType<QVector<int>>("QVector<int>"); // for building on Windows (qt 5.15.2)
    qRegisterMetaType<QCryptographicHash::Algorithm>("QCryptographicHash::Algorithm");
    qRegisterMetaType<View::ModelView>("View::ModelView");
    qRegisterMetaType<FileTypeList>("FileTypeList");
    qRegisterMetaType<Numbers>("Numbers");
    qRegisterMetaType<FileValues>("FileValues");
    qRegisterMetaType<MetaData::DbFileState>("MetaData::DbFileState");

    m_manager->moveToThread(m_thread);

    // when closing the thread
    connect(m_thread, &QThread::finished, m_thread, &QThread::deleteLater);
    connect(m_thread, &QThread::finished, m_manager, &Manager::deleteLater);

    // info and notifications
    connect(m_manager, &Manager::setStatusbarText, m_statusBar, &StatusBar::setStatusText);
    connect(m_manager, &Manager::showMessage, this, &MainWindow::showMessage);
    connect(m_manager, &Manager::folderChecked, ui->view, &View::setMismatchFiltering);
    connect(m_manager, &Manager::folderChecked, this, &MainWindow::showFolderCheckResult);
    connect(m_manager, &Manager::fileProcessed, this, &MainWindow::showFileCheckResult);
    connect(m_manager, &Manager::folderContentsListCreated, this, &MainWindow::showDialogContentsList);
    connect(m_manager, &Manager::dbCreationDataCollected, this, &MainWindow::showDialogDbCreation);
    connect(m_manager, &Manager::dbContentsListCreated, this, &MainWindow::showDialogDbContents);
    connect(m_manager, &Manager::mismatchFound, this, &MainWindow::setWinTitleMismatchFound);

    // results processing
    connect(m_manager, &Manager::setViewData, ui->view, &View::setData);
    connect(m_manager->m_dataMaintainer, &DataMaintainer::databaseUpdated, this, &MainWindow::showDbStatus);
    connect(m_manager->m_dataMaintainer, &DataMaintainer::numbersUpdated, this, &MainWindow::updatePermanentStatus);
    connect(m_manager->m_dataMaintainer, &DataMaintainer::numbersUpdated, this, &MainWindow::updateWindowTitle);
    connect(m_manager->m_dataMaintainer, &DataMaintainer::subDbForked, this, &MainWindow::promptOpenBranch);
    connect(m_manager->m_dataMaintainer, &DataMaintainer::failedDataCreation, this,
            [=]{ if (ui->view->isViewModel(ModelView::NotSet)) m_modeSelect->showFileSystem(); });
    connect(m_manager->m_dataMaintainer, &DataMaintainer::failedJsonSave, this, &MainWindow::dialogSaveJson);
    connect(m_manager->m_dataMaintainer, &DataMaintainer::dbFileStateChanged, this, &MainWindow::updateMenuActions);
    connect(m_manager->m_dataMaintainer, &DataMaintainer::dbFileStateChanged, this,
            [=]{ if (m_proc->isAwaiting(ProcState::AwaitingClosure)) close(); });

    // process status
    connect(m_manager->m_proc, &ProcState::stateChanged, this, [=]{ if (m_proc->isState(State::Idle)) ui->view->setViewProxy(); });
    connect(m_manager->m_proc, &ProcState::stateChanged, this, &MainWindow::updateButtonInfo);
    connect(m_manager->m_proc, &ProcState::progressStarted, ui->progressBar, &ProgressBar::start);
    connect(m_manager->m_proc, &ProcState::progressFinished, ui->progressBar, &ProgressBar::finish);
    connect(m_manager->m_proc, &ProcState::percentageChanged, ui->progressBar, &ProgressBar::setValue);
    connect(m_manager->m_proc, &ProcState::stateChanged, this, &MainWindow::updateStatusIcon);

    // <!> experimental
    connect(m_manager->m_proc, &ProcState::progressFinished, m_modeSelect, &ModeSelector::getInfoPathItem);

    // change view
    connect(m_manager, &Manager::switchToFsPrepared, this, &MainWindow::switchToFs);
    connect(ui->view, &View::switchedToFs, m_manager->m_dataMaintainer, &DataMaintainer::clearData);
    connect(ui->view, &View::modelChanged, m_manager, &Manager::modelChanged);
    connect(ui->view, &View::dataSetted, m_manager->m_dataMaintainer, &DataMaintainer::clearOldData);

    connect(ui->view, &View::dataSetted, this, &MainWindow::addRecentFile);

    m_thread->start();
}

void MainWindow::saveSettings()
{
    ui->view->saveHeaderState();
    m_settings->geometryMainWindow = saveGeometry();
    m_settings->saveSettings();
}

void MainWindow::switchToFs()
{
    ui->view->setFileSystemModel();
    m_proc->setAwaiting(ProcState::AwaitingNothing);
}

void MainWindow::showDbStatus()
{
    showDbStatusTab(DialogDbStatus::TabAutoSelect);
}

void MainWindow::showDbStatusTab(DialogDbStatus::Tabs tab)
{
    if (m_proc->isState(State::StartSilently)) {
        qDebug() << "MainWindow::showDbStatusTab | Rejected >> State::StartSilently";
        return;
    }

    clearDialogs();

    if (m_modeSelect->isMode(Mode::DbIdle | Mode::DbCreating)) {
        DialogDbStatus dialog(ui->view->m_data, this);
        dialog.setCurrentTab(tab);
        dialog.exec();

        QString &old_comment = ui->view->m_data->m_metadata.comment;
        const QString new_comment = dialog.getComment();

        if (new_comment != old_comment) {
            old_comment = new_comment;

            if (!m_modeSelect->isMode(Mode::DbCreating))
                m_manager->m_dataMaintainer->setDbFileState(DbFileState::NotSaved);

            qDebug() << "Comment updated";
        }
    }
}

void MainWindow::showDialogContentsList(const QString &folderName, const FileTypeList &extList)
{
    if (extList.isEmpty())
        return;

    DialogContentsList dialog(folderName, extList, this);
    dialog.setWindowIcon(m_modeSelect->m_icons.iconFolder());
    dialog.exec();
}

void MainWindow::showDialogDbCreation(const QString &folder, const QStringList &dbFiles, const FileTypeList &extList)
{
    if (extList.isEmpty())
        return;

    clearDialogs();

    if (!dbFiles.isEmpty()) {
        DialogExistingDbs dlg(dbFiles, this);
        if (dlg.exec() && !dlg.curFile().isEmpty()) {
            m_modeSelect->openJsonDatabase(pathstr::joinPath(folder, dlg.curFile()));
            return;
        }
    }

    DialogDbCreation dialog(folder, extList, this);
    dialog.setSettings(m_settings);
    dialog.setWindowTitle(QStringLiteral(u"New database..."));

    if (!dialog.exec())
        return;

    const FilterRule filterRule = dialog.resultFilter();
    const QString comment = dialog.getComment();

    if (filterRule || !dialog.isFilterCreating()) {
        m_modeSelect->processFolderChecksums(filterRule, comment);
    } else { // filter creation is enabled, BUT no suffix(type) is ​​selected
        QMessageBox msgBox(this);
        msgBox.setIconPixmap(m_modeSelect->m_icons.pixmap(Icons::Filter));
        msgBox.setWindowTitle(QStringLiteral(u"No filter specified"));
        msgBox.setText(QStringLiteral(u"File filtering is not set."));
        msgBox.setInformativeText(QStringLiteral(u"Continue with all files?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Abort);
        msgBox.setDefaultButton(QMessageBox::Yes);
        msgBox.button(QMessageBox::Yes)->setText(QStringLiteral(u"Continue"));

        if (msgBox.exec() == QMessageBox::Yes)
            m_modeSelect->processFolderChecksums(filterRule, comment);
    }
}

void MainWindow::showDialogDbContents(const QString &folderName, const FileTypeList &extList)
{
    if (extList.isEmpty())
        return;

    QString strWindowTitle = QStringLiteral(u"Database Contents");
    if (ui->view->m_data && ui->view->m_data->m_numbers.contains(FileStatus::Missing))
        strWindowTitle.append(QStringLiteral(u" [available ones]"));

    DialogContentsList dialog(folderName, extList, this);
    dialog.setWindowIcon(m_modeSelect->m_icons.icon(Icons::Database));
    dialog.setWindowTitle(strWindowTitle);
    dialog.exec();
}

void MainWindow::showFolderCheckResult(const Numbers &result, const QString &subFolder)
{
    clearDialogs();

    QMessageBox msgBox(this);

    QString titleText = result.contains(FileStatus::Mismatched) ? QStringLiteral(u"FAILED")
                                                                : QStringLiteral(u"Success");

    QString messageText = !subFolder.isEmpty() ? QString("Subfolder: %1\n\n").arg(subFolder)
                                               : QString();

    if (result.contains(FileStatus::Mismatched)) {
        if (m_modeSelect->isDbConst()) {
            msgBox.setStandardButtons(QMessageBox::Cancel);
        } else {
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.button(QMessageBox::Ok)->setText(QStringLiteral(u"Update"));
            msgBox.button(QMessageBox::Ok)->setIcon(m_modeSelect->m_icons.icon(FileStatus::Updated));
            msgBox.setDefaultButton(QMessageBox::Cancel);
        }

        msgBox.button(QMessageBox::Cancel)->setText(QStringLiteral(u"Continue"));
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
                               + QStringLiteral(u" remain unchecked."));
        }
    } else {
        messageText.append(QString("ALL %1 files passed verification.")
                            .arg(result.numberOf(FileStatus::CombMatched)));
    }

    FileStatus st_icon = result.contains(FileStatus::Mismatched) ? FileStatus::Mismatched
                                                                 : FileStatus::Matched;

    msgBox.setIconPixmap(m_modeSelect->m_icons.pixmap(st_icon));
    msgBox.setWindowTitle(titleText);
    msgBox.setText(messageText);

    const int ret = msgBox.exec();

    if (!m_modeSelect->isDbConst()
        && result.contains(FileStatus::Mismatched)
        && ret == QMessageBox::Ok)
    {
        m_modeSelect->updateDatabase(DbMod::DM_UpdateMismatches);
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
    DialogSettings dialog(m_settings, this);

    if (!dialog.exec())
        return;

    dialog.updateSettings();

    // switching "Detect Moved" cache
    if (ui->view->isViewDatabase()) {
        DataContainer *dc = ui->view->m_data;

        if (m_settings->detectMoved
            && dc->m_cacheMissing.isEmpty()
            && DataHelper::hasPossiblyMovedItems(dc))
        {
            m_manager->addTaskWithState(State::Idle, &Manager::cacheMissingItems);
        }
        else if (!m_settings->detectMoved
                   && !dc->m_cacheMissing.isEmpty())
        {
            dc->m_cacheMissing.clear();
        }
    }
}

void MainWindow::dialogChooseFolder()
{
    const QString path = QFileDialog::getExistingDirectory(this, QString(), QDir::homePath());

    if (!path.isEmpty()) {
        m_modeSelect->showFileSystem(path);
    }
}

void MainWindow::dialogOpenJson()
{
    const QString path = QFileDialog::getOpenFileName(this, "Select Veretino Database",
                                                      QDir::homePath(),
                                                      "Veretino DB (*.ver *.ver.json)");

    if (!path.isEmpty()) {
        m_modeSelect->openJsonDatabase(path);
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
    msgBox.setIconPixmap(m_modeSelect->m_icons.pixmap(Icons::AddFork));
    msgBox.setWindowTitle("A new Branch has been created");
    msgBox.setText("The subfolder data is forked:\n" + format::shortenPath(dbFilePath));
    msgBox.setInformativeText("Do you want to open it or stay in the current one?");
    msgBox.setStandardButtons(QMessageBox::Open | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::No)->setText("Stay");

    if (msgBox.exec() == QMessageBox::Open) {
        m_modeSelect->openJsonDatabase(dbFilePath);
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
            if (m_proc->isAwaiting()
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

        if (m_manager->m_dataMaintainer->saveJsonFile(pUnsavedJson)) {
            addRecentFile();
            break;
        }
    }

    delete pUnsavedJson;
    qDebug() << "dialogSaveJson:" << file_path;

    if (m_proc->isAwaiting(ProcState::AwaitingClosure)) {
        close();
    } else if (m_proc->isAwaiting(ProcState::AwaitingSwitchToFs)) {
        switchToFs();
    }
}

void MainWindow::updateButtonInfo()
{
    ui->button->setIcon(m_modeSelect->getButtonIcon());
    ui->button->setText(m_modeSelect->getButtonText());
    ui->button->setToolTip(m_modeSelect->getButtonToolTip());
}

void MainWindow::updateMenuActions()
{
    m_modeSelect->m_menuAct->actionShowFilesystem->setEnabled(!ui->view->isViewFileSystem());
    m_modeSelect->m_menuAct->actionSave->setEnabled(ui->view->isViewDatabase()
                                                    && DataHelper::isDbFileState(ui->view->m_data, DbFileState::NotSaved));
}

void MainWindow::updateStatusIcon()
{
    QIcon statusIcon;
    if (m_proc->isStarted()) {
        statusIcon = m_modeSelect->m_icons.icon(FileStatus::Calculating);
    }
    else if (ui->view->isViewFileSystem()) {
        statusIcon = m_modeSelect->m_icons.icon(ui->view->curAbsPath());
    }
    else if (ui->view->isViewDatabase()) {
        if (TreeModel::isFileRow(ui->view->curIndex()))
            statusIcon = m_modeSelect->m_icons.icon(TreeModel::itemFileStatus(ui->view->curIndex()));
        else
            statusIcon = m_modeSelect->m_icons.iconFolder();
    }

    m_statusBar->setStatusIcon(statusIcon);
}

void MainWindow::updatePermanentStatus()
{
    if (ui->view->isViewDatabase()) {
        if (m_modeSelect->isMode(Mode::DbCreating))
            m_statusBar->setModeDbCreating();
        else if (!m_proc->isStarted())
            m_statusBar->setModeDb(ui->view->m_data);
    } else {
        m_statusBar->clearButtons();
    }
}

void MainWindow::setWinTitleMismatchFound()
{
    static const QString str = tools::joinStrings(Lit::s_appName,
                                                  QStringLiteral(u"<!> mismatches found"),
                                                  Lit::s_sepStick);
    setWindowTitle(str);
    setWindowIcon(m_modeSelect->m_icons.icon(FileStatus::Mismatched));
}

void MainWindow::updateWindowTitle()
{
    if (ui->view->isViewDatabase()) {
        const DataContainer *data = ui->view->m_data;

        if (data->m_numbers.contains(FileStatus::Mismatched)) {
            setWinTitleMismatchFound();
            return;
        }

        const bool isVerified = DataHelper::isAllMatched(data);

        QString strAdd = isVerified ? QStringLiteral(u"✓ verified")
                                    : QStringLiteral(u"DB > ") + format::shortenPath(data->m_metadata.workDir);

        QIcon icn = isVerified ? m_modeSelect->m_icons.icon(FileStatus::Matched)
                               : IconProvider::appIcon();

        const QString strTitle = tools::joinStrings(Lit::s_appName, strAdd, Lit::s_sepStick);
        setWindowTitle(strTitle);
        setWindowIcon(icn);
    } else {
        setWindowTitle(Lit::s_appName);
        setWindowIcon(IconProvider::appIcon());
    }
}

void MainWindow::addRecentFile()
{
    if (ui->view->m_data) {
        const QString &filePath = ui->view->m_data->m_metadata.dbFilePath;
        if (QFileInfo::exists(filePath))
            m_settings->addRecentFile(filePath);
    }
}

void MainWindow::handlePathEdit()
{
    if (ui->pathEdit->text() == ui->view->curAbsPath()) {
        m_modeSelect->quickAction();
    } else {
        ui->view->setIndexByPath(ui->pathEdit->text().replace('\\', '/'));
    }
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
    if (!m_proc->isStarted() && ui->view->isViewDatabase()) {
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
    if (m_modeSelect->isMode(Mode::File | Mode::Folder)) {
        m_modeSelect->m_menuAct->menuAlgorithm(m_settings->algorithm())->exec(ui->button->mapToGlobal(point));
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
                m_modeSelect->openJsonDatabase(argPath);
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
            m_modeSelect->openJsonDatabase(path);
        } else {
            m_modeSelect->showFileSystem(path);
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        // Idle DB
        if (m_modeSelect->isMode(Mode::DbIdle)) {
            if (ui->view->isViewFiltered())
                ui->view->disableFilter();
            else if (QMessageBox::question(this, "Exit...", "Close the Database?") == QMessageBox::Yes)
                m_modeSelect->showFileSystem();
        }
        // Verifying/Updating (not creating) DB || filesystem process
        else if (m_modeSelect->isMode(Mode::DbProcessing | Mode::FileProcessing)) {
            m_modeSelect->promptProcessStop();
        }
        // other cases
        else if (m_modeSelect->promptProcessAbort()
                 && !ui->view->isViewFileSystem())
        {
            m_modeSelect->showFileSystem();
        }
    }
    // temporary solution until appropriate Actions are added
    else if (event->key() == Qt::Key_F1
             && m_modeSelect->isMode(Mode::DbIdle | Mode::DbCreating))
    {
        showDbStatus();
    }
    else if (event->key() == Qt::Key_F5
             && !m_proc->isStarted() && m_modeSelect->isMode(Mode::DbIdle))
    {
        m_modeSelect->resetDatabase();
    }
    // TMP ^^^

    QMainWindow::keyPressEvent(event);
}
