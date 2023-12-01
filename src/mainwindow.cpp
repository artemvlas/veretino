// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/veretino.png"));
    QThread::currentThread()->setObjectName("MAIN Thread");

    modeSelect = new ModeSelector(ui->treeView, settings_, this);
    ui->statusbar->addPermanentWidget(permanentStatus);

    connections();

    if (!argumentInput())
        ui->treeView->setFileSystemModel();

    ui->progressBar->setVisible(false);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
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

    // ModeSelector
    connect(ui->button, &QPushButton::clicked, modeSelect, &ModeSelector::doWork);
    connect(modeSelect, &ModeSelector::setButtonText, ui->button, &QPushButton::setText);

    //TreeView
    connect(ui->treeView, &View::keyEnterPressed, modeSelect, &ModeSelector::quickAction);
    connect(ui->treeView, &View::doubleClicked, modeSelect, &ModeSelector::quickAction);
    connect(ui->treeView, &View::customContextMenuRequested, modeSelect, &ModeSelector::createContextMenu_View);
    connect(ui->treeView, &View::pathChanged, ui->lineEdit, &QLineEdit::setText);
    connect(ui->treeView, &View::pathChanged, modeSelect, &ModeSelector::setMode);
    connect(ui->treeView, &View::modelChanged, this, [=](ModelView modelView){ui->lineEdit->setEnabled(modelView == ModelView::FileSystem);});
    connect(ui->treeView, &View::showMessage, this, &MainWindow::showMessage);

    connect(ui->lineEdit, &QLineEdit::returnPressed, this, [=]{ui->treeView->setIndexByPath(ui->lineEdit->text().replace("\\", "/"));});

    //menu actions
    connect(ui->actionSettings, &QAction::triggered, this, &MainWindow::dialogSettings);
    connect(ui->actionOpenFolder, &QAction::triggered, this, &MainWindow::dialogOpenFolder);
    connect(ui->actionOpenJson, &QAction::triggered, this, &MainWindow::dialogOpenJson);
    connect(ui->actionShowFs, &QAction::triggered, modeSelect, &ModeSelector::showFileSystem);
    connect(ui->actionAbout, &QAction::triggered, this, [=]{aboutDialog about; about.exec();});
}

void MainWindow::connectManager()
{
    qRegisterMetaType<QVector<int>>("QVector<int>"); // for building on Windows (qt 5.15.2)
    qRegisterMetaType<QSet<FileStatus>>("QSet<FileStatus>");
    qRegisterMetaType<QCryptographicHash::Algorithm>("QCryptographicHash::Algorithm");
    qRegisterMetaType<ModelView>("ModelView");

    manager->moveToThread(thread);

    // when closing the thread
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(thread, &QThread::finished, manager, &Manager::deleteLater);

    // signals for execution tasks
    connect(modeSelect, &ModeSelector::parseJsonFile, manager, &Manager::createDataModel);
    connect(modeSelect, &ModeSelector::processFolderSha, manager, &Manager::processFolderSha);
    connect(modeSelect, &ModeSelector::processFileSha, manager, &Manager::processFileSha);
    connect(modeSelect, &ModeSelector::verify, manager, &Manager::verify);
    connect(modeSelect, &ModeSelector::updateNewLost, manager, &Manager::updateNewLost);
    connect(modeSelect, &ModeSelector::updateMismatch, manager, &Manager::updateMismatch);
    connect(modeSelect, &ModeSelector::checkSummaryFile, manager, &Manager::checkSummaryFile); // check *.sha1 *.sha256 *.sha512 summaries
    connect(modeSelect, &ModeSelector::checkFile, manager, qOverload<const QString&, const QString&>(&Manager::checkFile));

    // cancel process
    connect(modeSelect, &ModeSelector::cancelProcess, manager, &Manager::cancelProcess, Qt::DirectConnection);

    // info and notifications
    connect(manager, SIGNAL(setStatusbarText(QString)), ui->statusbar, SLOT(showMessage(QString)));
    connect(manager, &Manager::setPermanentStatus, permanentStatus, &QLabel::setText);
    connect(manager, &Manager::showMessage, this, &MainWindow::showMessage);
    connect(modeSelect, &ModeSelector::getPathInfo, manager, &Manager::getPathInfo);
    connect(modeSelect, &ModeSelector::getIndexInfo, manager, &Manager::getIndexInfo);
    connect(modeSelect, &ModeSelector::folderContentsByType, manager, &Manager::folderContentsByType);
    connect(modeSelect, &ModeSelector::dbStatus, manager->dataMaintainer, &DataMaintainer::dbStatus);

    // results processing
    connect(manager, &Manager::setTreeModel, ui->treeView, &View::setTreeModel);
    connect(manager, &Manager::setViewData, ui->treeView, &View::setData);
    connect(manager, &Manager::toClipboard, this, [=](const QString &text){QGuiApplication::clipboard()->setText(text);}); //send text to system clipboard
    connect(manager->dataMaintainer, &DataMaintainer::dataUpdated, modeSelect, &ModeSelector::setMode);

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

void MainWindow::dialogSettings()
{
    settingDialog dialog(settings_);

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

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    QString path = e->mimeData()->urls().first().toLocalFile();

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
