#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowIcon(QIcon(":/veretino.png"));

    ui->statusbar->addPermanentWidget(permanentStatus);

    connections();

    QThread::currentThread()->setObjectName("MAIN Thread");

    if (!argumentInput())
        ui->treeView->setFileSystemModel();

    ui->progressBar->setVisible(false);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
}

MainWindow::~MainWindow()
{
    emit cancelProcess();

    thread->quit();
    thread->wait();

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)  // if a computing process is running, show a hint when user wants to close the app
{
    if (viewMode == Mode::Processing && !processAbortPrompt())
        event->ignore();
    else
        event->accept();
}

void MainWindow::connections()
{
    connectManager();

    connect(ui->button, &QPushButton::clicked, this , &MainWindow::doWork);
    connect(this, &MainWindow::cancelProcess, this, [=]{if (viewMode == Mode::Processing) setMode(Mode::EndProcess);});
    connect(this, &MainWindow::getItemInfo, this, &MainWindow::cancelProcess);
    connect(this, &MainWindow::folderContentsByType, this, &MainWindow::cancelProcess);

    //TreeView
    connect(ui->treeView, &View::keyEnterPressed, this, &MainWindow::quickAction);
    connect(ui->treeView, &View::doubleClicked, this, &MainWindow::quickAction);
    connect(ui->treeView, &View::customContextMenuRequested, this, &MainWindow::onCustomContextMenu);
    connect(ui->treeView, &View::pathChanged, this, [=](const QString &path){curPath = path; ui->lineEdit->setText(path); if (viewMode != Mode::Processing) emit getItemInfo(path);});
    connect(ui->treeView, &View::setMode, this, &MainWindow::setMode);
    connect(ui->treeView, &View::modelChanged, ui->lineEdit, &QLineEdit::setEnabled);
    connect(ui->treeView, &View::showMessage, this, &MainWindow::showMessage);

    connect(ui->lineEdit, &QLineEdit::returnPressed, this, [=]{ui->treeView->setIndexByPath(ui->lineEdit->text().replace("\\", "/"));});

    //menu actions
    connect(ui->actionOpenFolder, &QAction::triggered, this, [=]{QString path = QFileDialog::getExistingDirectory(this, "Open folder", homePath);
        if (!path.isEmpty()) {if (viewMode == Mode::Processing) emit cancelProcess(); if (!ui->treeView->isViewFileSystem()) ui->treeView->setFileSystemModel(); ui->treeView->setIndexByPath(path);}});

    connect(ui->actionOpenJson, &QAction::triggered, this, [=]{QString path = QFileDialog::getOpenFileName(this, "Open Veretino database", homePath, "DB Files (*.ver.json)");
        if (!path.isEmpty()) {if (viewMode == Mode::Processing) emit cancelProcess(); emit parseJsonFile(path);}});

    connect(ui->actionShowFs, &QAction::triggered, this, [=]{if (viewMode == Mode::Processing) {emit cancelProcess();} ui->treeView->setFileSystemModel();});

    connect(ui->actionSettings, &QAction::triggered, this, [=]{settingDialog dialog (settings_); if (dialog.exec() == QDialog::Accepted){
                                                            dialog.updateSettings(); setMode(viewMode);}}); // "setMode" changes the text on button

    connect(ui->actionAbout, &QAction::triggered, this, [=]{aboutDialog about; about.exec();});

}

void MainWindow::connectManager()
{
    manager->moveToThread(thread);

    // when closing the thread
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(thread, &QThread::finished, manager, &Manager::deleteLater);

    // signals for execution tasks
    qRegisterMetaType<QCryptographicHash::Algorithm>("QCryptographicHash::Algorithm");
    connect(this, &MainWindow::parseJsonFile, manager, &Manager::createDataModel);
    connect(this, &MainWindow::processFolderSha, manager, &Manager::processFolderSha);
    connect(this, &MainWindow::processFileSha, manager, &Manager::processFileSha);
    connect(this, &MainWindow::verifyFileList, manager, &Manager::verifyFileList);
    connect(this, &MainWindow::updateNewLost, manager, &Manager::updateNewLost);
    connect(this, &MainWindow::updateMismatch, manager, &Manager::updateMismatch);
    connect(this, &MainWindow::checkSummaryFile, manager, &Manager::checkSummaryFile); // check *.sha1 *.sha256 *.sha512 summaries
    connect(this, &MainWindow::checkFile, manager, qOverload<const QString&, const QString&>(&Manager::checkFile));
    connect(this, &MainWindow::checkCurrentItemSum, manager, &Manager::checkCurrentItemSum);
    connect(this, &MainWindow::copyStoredChecksum, manager, &Manager::copyStoredChecksum);

    // cancel process
    connect(this, &MainWindow::cancelProcess, manager, &Manager::cancelProcess, Qt::DirectConnection);

    // info and notifications
    connect(manager, SIGNAL(setStatusbarText(QString)), ui->statusbar, SLOT(showMessage(QString)));
    connect(manager, &Manager::setPermanentStatus, permanentStatus, &QLabel::setText);
    connect(manager, &Manager::showMessage, this, &MainWindow::showMessage);
    connect(this, &MainWindow::getItemInfo, manager, &Manager::getItemInfo);
    connect(this, &MainWindow::folderContentsByType, manager, &Manager::folderContentsByType);
    connect(this, &MainWindow::dbStatus, manager, &Manager::dbStatus);

    // results processing
    connect(manager, &Manager::setModel, ui->treeView, &View::setTreeModel); //set the tree model created by Manager
    connect(manager, &Manager::toClipboard, this, [=](const QString &text){QGuiApplication::clipboard()->setText(text);}); //send text to system clipboard
    connect(manager, &Manager::workDirChanged, this, [=](const QString &path){ui->treeView->workDir = path;});

    // process status
    connect(manager, &Manager::donePercents, ui->progressBar, &QProgressBar::setValue);
    connect(manager, &Manager::timeLeft, this, [=](const QString &str){ui->progressBar->setFormat(QString("%p% | %1").arg(str));});

    // transfer settings and modes
    qRegisterMetaType<Mode::Modes>("Mode::Modes");
    connect(manager, &Manager::setMode, this, &MainWindow::setMode);

    // change view
    connect(this, &MainWindow::resetDatabase, manager, &Manager::resetDatabase); // reopening and reparsing current database
    connect(this, &MainWindow::showNewLostOnly, manager, &Manager::showNewLostOnly);
    connect(ui->treeView, &View::modelChanged, manager, &Manager::isViewFS);
    connect(this, &MainWindow::showAll, manager, &Manager::showAll);

    thread->start();
}

void MainWindow::onCustomContextMenu(const QPoint &point)
{
    using namespace Mode;
    QModelIndex index = ui->treeView->indexAt(point);

    QMenu *contextMenu = new QMenu(ui->treeView);
    connect(contextMenu, &QMenu::aboutToHide, contextMenu, &QMenu::deleteLater);

    if (ui->treeView->isViewFileSystem()) {
        contextMenu->addAction("to Home", this, [=]{ui->treeView->setIndexByPath(homePath);});
        contextMenu->addSeparator();

        if (viewMode == Processing)
            contextMenu->addAction("Cancel operation", this, &MainWindow::cancelProcess);

        else if (index.isValid()) {

            if (viewMode == Folder) {
                contextMenu->addAction("Folder Contents By Type", this, [=]{emit folderContentsByType(curPath);});
                contextMenu->addSeparator();
                contextMenu->addAction(QString("Compute %1 for all files in folder").arg(format::algoToStr(settings_->algorithm)), this,
                                                      [=]{emit cancelProcess(); emit processFolderSha(curPath, settings_->algorithm);});
            }
            else if (viewMode == File) {
                QString clipboardText = QGuiApplication::clipboard()->text();
                if (tools::canBeChecksum(clipboardText)) {
                    contextMenu->addAction("Check the file by checksum: " + format::shortenString(clipboardText), this, [=]{emit checkFile(curPath, clipboardText);});
                    contextMenu->addSeparator();
                }
                contextMenu->addAction("SHA-1 --> *.sha1", this, [=]{emit processFileSha(curPath, QCryptographicHash::Sha1);});
                contextMenu->addAction("SHA-256 --> *.sha256", this, [=]{emit processFileSha(curPath, QCryptographicHash::Sha256);});
                contextMenu->addAction("SHA-512 --> *.sha512", this, [=]{emit processFileSha(curPath, QCryptographicHash::Sha512);});
            }
            else if (viewMode == DbFile)
                contextMenu->addAction("Open DataBase", this, &MainWindow::doWork);
            else if (viewMode == SumFile) {
                contextMenu->addAction("Check the Checksum", this, [=]{emit checkSummaryFile(curPath);});
            }
        }
    }

    else {
        if (viewMode == Processing) {
            contextMenu->addAction("Cancel operation", this, &MainWindow::cancelProcess);
            contextMenu->addAction("Cancel and Back to FileSystem View", this, [=]{emit cancelProcess(); ui->treeView->setFileSystemModel();});
        }

        else {
            contextMenu->addAction("Status", this, &MainWindow::dbStatus);
            contextMenu->addAction("Reset", this, &MainWindow::resetDatabase);
            contextMenu->addSeparator();
            contextMenu->addAction("Show FileSystem", ui->treeView, &View::setFileSystemModel);
            contextMenu->addSeparator();

            if (viewMode == UpdateMismatch)
                contextMenu->addAction("Update the Database with new checksums", this, &MainWindow::updateMismatch);

            else if (viewMode == ModelNewLost) {
                contextMenu->addAction("Show New/Lost only", this, &MainWindow::showNewLostOnly);
                contextMenu->addAction("Update the DataBase with New/Lost files", this, &MainWindow::updateNewLost);
                contextMenu->addSeparator();
            }

            if (viewMode == Model || viewMode == ModelNewLost) {
                if (index.isValid()) {
                    QFileInfo pathInfo (paths::joinPath(ui->treeView->workDir, curPath));
                    if (pathInfo.isFile()) {
                        contextMenu->addAction("Check current file", this, [=]{emit checkCurrentItemSum(curPath);});
                        contextMenu->addAction("Copy stored checksum to clipboard", this, [=]{emit copyStoredChecksum(curPath);});
                    }
                    else if (pathInfo.isDir()) {
                        contextMenu->addAction("Check current Subfolder", this, [=]{emit verifyFileList(curPath);});
                    }
                }

                contextMenu->addAction("Check ALL files against stored checksums", this, [=]{emit verifyFileList();});
            }
            contextMenu->addSeparator();
            contextMenu->addAction("Show All", this, &MainWindow::showAll);
        }
        contextMenu->addSeparator();
        contextMenu->addAction("Collapse all", ui->treeView, &QTreeView::collapseAll);
        contextMenu->addAction("Expand all", ui->treeView, &QTreeView::expandAll);
    }

    contextMenu->exec(ui->treeView->viewport()->mapToGlobal(point));
}

void MainWindow::setMode(Mode::Modes mode)
{
    //qDebug() << "MainWindow::setMode | mode:" << mode;
    using namespace Mode;

    if (viewMode == Processing && mode != EndProcess)
        return;

    if (mode == EndProcess) {
        viewMode = NoMode;
        ui->progressBar->setVisible(false);
        ui->progressBar->resetFormat();
        ui->progressBar->setValue(0);
        if (ui->treeView->isViewFileSystem()) {
            ui->treeView->pathAnalyzer(curPath);
            return;
        }
        else {
            if (previousViewMode == Model) {
                viewMode = Model;
                qDebug()<< "previousViewMode setted:" << previousViewMode;
            }
            else if (previousViewMode == ModelNewLost) {
                viewMode = ModelNewLost;
                qDebug()<< "previousViewMode setted:" << previousViewMode;
            }
        }
    }
    else {
        if (viewMode != NoMode)
            previousViewMode = viewMode;
        viewMode = mode;
    }

    switch (viewMode) {
    case Folder:
        ui->button->setText(format::algoToStr(settings_->algorithm).append(": Folder"));
        break;
    case File:
        ui->button->setText(format::algoToStr(settings_->algorithm).append(": File"));
        break;
    case DbFile:
        ui->button->setText("Open DataBase");
        break;
    case SumFile:
        ui->button->setText("Check");
        break;
    case Model:
        ui->button->setText("Verify All");
        break;
    case ModelNewLost:
        ui->button->setText("Update New/Lost");
        break;
    case UpdateMismatch:
        ui->button->setText("Update");
        break;
    case Processing:
        ui->progressBar->setVisible(true);
        ui->button->setText("Cancel");
        break;
    default:
        qDebug() << "MainWindow::setMode | WRONG MODE" << mode;
        break;
    }
}

void MainWindow::doWork()
{
    using namespace Mode;
    switch (viewMode) {
    case Folder:
        emit cancelProcess();
        emit processFolderSha(curPath, settings_->algorithm);
        break;
    case File:
        emit processFileSha(curPath, settings_->algorithm);
        break;
    case DbFile:
        emit parseJsonFile(curPath);
        break;
    case SumFile:
        emit checkSummaryFile(curPath);
        break;
    case Model:
        emit verifyFileList();
        break;
    case ModelNewLost:
        emit updateNewLost();
        break;
    case UpdateMismatch:
        emit updateMismatch();
        break;
    case Processing:
        emit cancelProcess();
        break;
    default:
        qDebug() << "MainWindow::doWork() | Wrong viewMode:" << viewMode;
        break;
    }
}

void MainWindow::quickAction()
{
    using namespace Mode;
    switch (viewMode) {
    case File:
        emit processFileSha(curPath, settings_->algorithm, false, true);
        break;
    case DbFile:
        emit parseJsonFile(curPath);
        break;
    case SumFile:
        emit checkSummaryFile(curPath);
        break;
    case Model:
        if (QFileInfo(paths::joinPath(ui->treeView->workDir, curPath)).isFile())
            emit checkCurrentItemSum(curPath);
        break;
    case ModelNewLost:
        if (QFileInfo(paths::joinPath(ui->treeView->workDir, curPath)).isFile())
            emit checkCurrentItemSum(curPath);
        break;
    case UpdateMismatch:
        if (QFileInfo(paths::joinPath(ui->treeView->workDir, curPath)).isFile())
            emit checkCurrentItemSum(curPath);
        break;
    default: break;
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
                emit parseJsonFile(argPath);
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
        if (viewMode == Mode::Processing) {
            if (processAbortPrompt())
                emit cancelProcess();
            else
                return;
        }
        if (tools::isDatabaseFile(path)) {
            emit parseJsonFile(path);
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
        if (viewMode == Mode::Processing) {
            if (processAbortPrompt())
                emit cancelProcess();
        }
        else if (!ui->treeView->isViewFileSystem())
            ui->treeView->setFileSystemModel();
    }

    //QMainWindow::keyPressEvent(event);
}
