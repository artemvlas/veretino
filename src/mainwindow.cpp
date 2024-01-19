/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "dbstatusdialog.h"
#include "foldercontentsdialog.h"
#include <QClipboard>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/veretino.png"));
    QThread::currentThread()->setObjectName("MAIN Thread");

    modeSelect = new ModeSelector(ui->treeView, ui->button, settings_, this);
    ui->statusbar->addPermanentWidget(permanentStatus);

    connections();

    if (!argumentInput())
        ui->treeView->setFileSystemModel();

    ui->progressBar->setVisible(false);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->button->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->menuFile->addActions(modeSelect->menuFileActions);
    ui->menuFile->insertSeparator(modeSelect->actionOpenSettingsDialog);
}

MainWindow::~MainWindow()
{
    emit modeSelect->cancelProcess();

    thread->quit();
    thread->wait();

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)  // if a computing process is running, show a hint when user wants to close the app
{
    modeSelect->isProcessing() && !processAbortPrompt() ? event->ignore()
                                                        : event->accept();
}

void MainWindow::connections()
{
    connectManager();

    // Push Button
    connect(ui->button, &QPushButton::clicked, modeSelect, &ModeSelector::doWork);
    connect(ui->button, &QPushButton::customContextMenuRequested, modeSelect, &ModeSelector::createContextMenu_Button);

    //TreeView
    connect(ui->treeView, &View::keyEnterPressed, modeSelect, &ModeSelector::quickAction);
    connect(ui->treeView, &View::doubleClicked, modeSelect, &ModeSelector::quickAction);
    connect(ui->treeView, &View::customContextMenuRequested, modeSelect, &ModeSelector::createContextMenu_View);
    connect(ui->treeView, &View::pathChanged, ui->pathEdit, &QLineEdit::setText);
    connect(ui->treeView, &View::pathChanged, modeSelect, &ModeSelector::setMode);
    connect(ui->treeView, &View::modelChanged, this, [=](ModelView modelView){ui->pathEdit->setEnabled(modelView == ModelView::FileSystem);});
    connect(ui->treeView, &View::showMessage, this, &MainWindow::showMessage);
    connect(ui->treeView, &View::showDbStatus, this, &MainWindow::showDbStatus);

    connect(ui->pathEdit, &QLineEdit::returnPressed, this, &MainWindow::handlePathEdit);
    connect(permanentStatus, &ClickableLabel::doubleClicked, this, &MainWindow::showDbStatus);

    //menu actions
    connect(modeSelect->actionOpenSettingsDialog, &QAction::triggered, this, &MainWindow::dialogSettings);
    connect(modeSelect->actionOpenFolder, &QAction::triggered, this, &MainWindow::dialogOpenFolder);
    connect(modeSelect->actionOpenDatabaseFile, &QAction::triggered, this, &MainWindow::dialogOpenJson);
    connect(ui->actionAbout, &QAction::triggered, this, [=]{aboutDialog about; about.exec();});
}

void MainWindow::connectManager()
{
    qRegisterMetaType<QVector<int>>("QVector<int>"); // for building on Windows (qt 5.15.2)
    qRegisterMetaType<QSet<FileStatus>>("QSet<FileStatus>");
    qRegisterMetaType<QCryptographicHash::Algorithm>("QCryptographicHash::Algorithm");
    qRegisterMetaType<ModelView>("ModelView");
    qRegisterMetaType<QList<ExtNumSize>>("QList<ExtNumSize>");
    qRegisterMetaType<MetaData>("MetaData");

    manager->moveToThread(thread);

    // when closing the thread
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(thread, &QThread::finished, manager, &Manager::deleteLater);

    // cancel process
    connect(modeSelect, &ModeSelector::cancelProcess, manager, &Manager::cancelProcess, Qt::DirectConnection);

    // signals for execution tasks
    connect(modeSelect, &ModeSelector::parseJsonFile, manager, &Manager::createDataModel);
    connect(modeSelect, &ModeSelector::processFolderSha, manager, &Manager::processFolderSha);
    connect(modeSelect, &ModeSelector::processFileSha, manager, &Manager::processFileSha);
    connect(modeSelect, &ModeSelector::verify, manager, &Manager::verify);
    connect(modeSelect, &ModeSelector::updateNewLost, manager, &Manager::updateNewLost);
    connect(modeSelect, &ModeSelector::updateMismatch, manager, &Manager::updateMismatch);
    connect(modeSelect, &ModeSelector::checkSummaryFile, manager, &Manager::checkSummaryFile); // check *.sha1 *.sha256 *.sha512 summaries
    connect(modeSelect, &ModeSelector::checkFile, manager, qOverload<const QString&, const QString&>(&Manager::checkFile));

    // info and notifications
    connect(manager, SIGNAL(setStatusbarText(QString)), ui->statusbar, SLOT(showMessage(QString)));
    connect(manager, &Manager::setPermanentStatus, permanentStatus, &QLabel::setText);
    connect(manager, &Manager::showMessage, this, &MainWindow::showMessage);
    connect(manager, &Manager::toClipboard, this, [=](const QString &text){QGuiApplication::clipboard()->setText(text);});
    connect(modeSelect, &ModeSelector::getPathInfo, manager, &Manager::getPathInfo);
    connect(modeSelect, &ModeSelector::getIndexInfo, manager, &Manager::getIndexInfo);
    connect(modeSelect, &ModeSelector::makeFolderContentsList, manager, &Manager::makeFolderContentsList);
    connect(modeSelect, &ModeSelector::makeFolderContentsFilter, manager, &Manager::makeFolderContentsFilter);
    connect(manager, &Manager::folderContentsListCreated, this, &MainWindow::showFolderContentsDialog);
    connect(manager, &Manager::folderContentsFilterCreated, this, &MainWindow::showFilterCreationDialog);

    // results processing
    connect(manager, &Manager::setTreeModel, ui->treeView, &View::setTreeModel);
    connect(manager, &Manager::setViewData, ui->treeView, &View::setData);
    connect(manager->dataMaintainer, &DataMaintainer::databaseUpdated, modeSelect, &ModeSelector::setMode);
    connect(manager->dataMaintainer, &DataMaintainer::databaseUpdated, this, &MainWindow::showDbStatus);

    // process status
    connect(manager, &Manager::processing, this, &MainWindow::setProgressBar);
    connect(manager, &Manager::processing, modeSelect, &ModeSelector::processing);
    connect(manager, &Manager::donePercents, ui->progressBar, &QProgressBar::setValue);
    connect(manager, &Manager::procStatus, ui->progressBar, &QProgressBar::setFormat);

    // change view
    connect(modeSelect, &ModeSelector::resetDatabase, manager, &Manager::resetDatabase); // reopening and reparsing current database
    connect(modeSelect, &ModeSelector::restoreDatabase, manager, &Manager::restoreDatabase);
    connect(ui->treeView, &View::modelChanged, manager, &Manager::modelChanged);
    connect(ui->treeView, &View::dataSetted, manager->dataMaintainer, &DataMaintainer::clearOldData);

    thread->start();
}

void MainWindow::showDbStatus()
{
    if (ui->treeView->data_ && !modeSelect->isProcessing()) {
        DbStatusDialog dbStatusDialog(ui->treeView->data_);
        dbStatusDialog.exec();
    }
}

void MainWindow::showFolderContentsDialog(const QString &folderName, const QList<ExtNumSize> &extList)
{
    if (!extList.isEmpty()) {
        FolderContentsDialog dialog(folderName, extList);
        if (dialog.exec() == QDialog::Accepted) {
            FilterRule filter = dialog.resultFilter();
            if (!filter.isFilter(FilterRule::NotSet))
                settings_->filter = filter;
        }
    }
}

void MainWindow::showFilterCreationDialog(const QString &folderName, const QList<ExtNumSize> &extList)
{
    if (!extList.isEmpty()) {
        FolderContentsDialog dialog(folderName, extList);
        dialog.setFilterCreatingEnabled();
        FilterRule filter;

        if (dialog.exec() == QDialog::Accepted) {
            filter = dialog.resultFilter();
        }

        modeSelect->processFolderChecksums(filter);
    }
}

void MainWindow::dialogSettings()
{
    SettingsDialog dialog(settings_);

    if (dialog.exec() == QDialog::Accepted) {
        dialog.updateSettings();
        modeSelect->setMode(); // "setMode" changes the text on button
    }
}

void MainWindow::dialogOpenFolder()
{
    QString path = QFileDialog::getExistingDirectory(this, "Open folder", QDir::homePath());

    if (!path.isEmpty()) {
        if (modeSelect->isProcessing())
            emit modeSelect->cancelProcess();

        if (!ui->treeView->isViewFileSystem())
            ui->treeView->setFileSystemModel();

        ui->treeView->setIndexByPath(path);
    }
}

void MainWindow::dialogOpenJson()
{
    QString path = QFileDialog::getOpenFileName(this, "Open Veretino database", QDir::homePath(), "DB Files (*.ver.json)");

    if (!path.isEmpty()) {
        if (modeSelect->isProcessing())
            emit modeSelect->cancelProcess();

        emit modeSelect->parseJsonFile(path);
    }
}

void MainWindow::showMessage(const QString &message, const QString &title)
{
    QMessageBox messageBox;

    if (title.toLower() == "error" || title.toLower() == "failed")
        messageBox.critical(0, title, message);
    else if (title.toLower() == "warning")
        messageBox.warning(0, title, message);
    else
        messageBox.information(0, title, message);
}

void MainWindow::setProgressBar(bool processing, bool visible)
{
    ui->progressBar->setVisible(processing && visible);
    ui->progressBar->setValue(0);
    ui->progressBar->resetFormat();
}

void MainWindow::handlePathEdit()
{
    (ui->pathEdit->text() == ui->treeView->curPathFileSystem) ? modeSelect->quickAction()
                                                              : ui->treeView->setIndexByPath(ui->pathEdit->text().replace("\\", "/"));
}

bool MainWindow::processAbortPrompt()
{
    return (QMessageBox::Yes == QMessageBox::question(this, "Processing...", "Abort current process?", QMessageBox::Yes | QMessageBox::No));
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
        if (modeSelect->isProcessing()) {
            if (processAbortPrompt())
                emit modeSelect->cancelProcess();
            else
                return;
        }
        if (tools::isDatabaseFile(path)) {
            emit modeSelect->parseJsonFile(path);
        }
        else {
            if (!ui->treeView->isViewFileSystem())
                ui->treeView->setFileSystemModel();

            ui->treeView->setIndexByPath(path);
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        if (modeSelect->isProcessing()) {
            if (processAbortPrompt())
                emit modeSelect->cancelProcess();
        }
        else if (!ui->treeView->isViewFileSystem())
            ui->treeView->setFileSystemModel();
    }

    //QMainWindow::keyPressEvent(event);
}
