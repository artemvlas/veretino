#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowIcon(QIcon(":/veretino.png"));

    connections();

    QThread::currentThread()->setObjectName("MAIN Thread");
    qDebug() << "Main thread: " << QThread::currentThread()->objectName();

    setSettings(); // set initial values to <settings> Map. In future versions, values from file

    if (!argumentInput())
        ui->treeView->setFileSystemModel();

    ui->progressBar->setVisible(false);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
}

MainWindow::~MainWindow()
{
    if (viewMode == "processing") {
        emit cancelProcess();
        qDebug()<<"Current Processing canceled due app closed";
    }

    thread->quit();
    while(!thread->isFinished());

    delete thread;
    delete ui;
}

void MainWindow::connections()
{
    connectManager();

    connect(ui->button, &QPushButton::clicked, this , &MainWindow::doWork);
    connect(this, &MainWindow::cancelProcess, [this]{setMode("endProcess");});

    //TreeView
    connect(ui->treeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onCustomContextMenu(QPoint)));
    connect(ui->treeView, &View::pathChanged, ui->lineEdit, &QLineEdit::setText);
    connect(ui->treeView, &View::pathChanged, this, [=](const QString &path){curPath = path; if(ui->treeView->isViewFileSystem()) emit getFInfo(path);});
    connect(ui->treeView, &View::setMode, this, &MainWindow::setMode);
    connect(ui->treeView, &View::modelChanged, this, [=]{if(ui->treeView->isViewFileSystem()) ui->lineEdit->setEnabled(true); else ui->lineEdit->setDisabled(true);});
    connect(ui->treeView, &View::showMessage, this, &MainWindow::showMessage);
    connect(ui->treeView, &View::doubleClicked, this, [=]{if(viewMode == "db") emit parseJsonFile(curPath); else if (viewMode == "sum") emit checkFileSummary(curPath);});

    connect(ui->lineEdit, &QLineEdit::returnPressed, this, [=]{ui->treeView->setIndexByPath(ui->lineEdit->text());});

    //menu actions
    connect(ui->actionOpenFolder, &QAction::triggered, this, [=]{QString path = QFileDialog::getExistingDirectory(this,"Open folder",homePath);
        if(path != nullptr) {if (viewMode == "processing") emit cancelProcess(); if(!ui->treeView->isViewFileSystem()) ui->treeView->setFileSystemModel(); ui->treeView->setIndexByPath(path);}});

    connect(ui->actionOpenJson, &QAction::triggered, this, [=]{QString path = QFileDialog::getOpenFileName(this, tr("Open Veretino database"), homePath, tr("DB Files (*.ver.json)"));
        if(path != nullptr) {if (viewMode == "processing") emit cancelProcess(); emit parseJsonFile(path);}});

    connect(ui->actionShowFs, &QAction::triggered, this, [=]{if (viewMode == "processing") {emit cancelProcess();} ui->treeView->setFileSystemModel();});

    connect(ui->actionSettings, &QAction::triggered, this, [=]{settingDialog dialog (settings); if(dialog.exec() == QDialog::Accepted){
                settings = dialog.getSettings(); setMode(viewMode); emit settingsChanged(settings);}}); //"setMode" changes the text on button

    connect(ui->actionAbout, &QAction::triggered, this, [=]{aboutDialog about; about.exec();});

}

void MainWindow::connectManager()
{
    //qRegisterMetaType <TreeModel*> ("tree");

    manager->moveToThread(thread);

    //when closing the thread
    connect(thread, SIGNAL(finished()), manager, SLOT(deleteLater()));

    //signals for execution tasks
    connect(this, SIGNAL(parseJsonFile(QString)), manager, SLOT(makeJsonModel(QString)));
    connect(this, SIGNAL(processFolderSha(QString,int)), manager, SLOT(processFolderSha(QString,int)));
    connect(this, SIGNAL(processFileSha(QString,int)), manager, SLOT(processFileSha(QString,int)));
    connect(this, SIGNAL(getFInfo(QString)), manager, SLOT(getFInfo(QString)));
    connect(this, SIGNAL(verifyFileList()), manager, SLOT(verifyFileList()));
    connect(this, SIGNAL(updateNewLost()), manager, SLOT(updateNewLost()));
    connect(this, SIGNAL(updateMismatch()), manager, SLOT(updateMismatch()));
    connect(this, SIGNAL(checkFileSummary(QString)), manager, SLOT(checkFileSummary(QString))); // check *.sha1 *.sha256 *.sha512 summaries
    connect(this, SIGNAL(checkCurrentItemSum(QString)), manager, SLOT(checkCurrentItemSum(QString)));

    //cancel process
    connect(this, &MainWindow::cancelProcess, manager, &Manager::cancelProcess, Qt::DirectConnection);

    //info and notifications
    connect(manager, SIGNAL(status(QString)), ui->statusbar, SLOT(showMessage(QString)));
    connect(manager,SIGNAL(showMessage(QString,QString)),this,SLOT(showMessage(QString,QString)));

    //results processing
    connect(manager, &Manager::completeTreeModel, ui->treeView, &View::smartSetModel); //set the tree model created by Manager
    connect(manager, &Manager::resetView, ui->treeView, &View::setFileSystemModel);
    connect(manager, &Manager::toClipboard, this, [=](const QString &text){QGuiApplication::clipboard()->setText(text);}); //send text to system clipboard

    //process status
    connect(manager, &Manager::donePercents, ui->progressBar, &QProgressBar::setValue);
    connect(manager, &Manager::donePercents, this, [=](const int &done){timeLeft(done);});

    //transfer settings and modes
    connect(manager, &Manager::setMode, this, &MainWindow::setMode);
    connect(this, &MainWindow::settingsChanged, manager, &Manager::getSettings); // send settings Map

    // change view
    connect(this, &MainWindow::resetDatabase, manager, &Manager::resetDatabase); // reopening and reparsing current database
    connect(this, &MainWindow::showNewLostOnly, manager, &Manager::showNewLostOnly);

    thread->start();
}

void MainWindow::onCustomContextMenu(const QPoint &point)
{
    QModelIndex index = ui->treeView->indexAt(point);
    QString path = curPath;

    QMenu *contextMenu = new QMenu(ui->treeView);

    if (ui->treeView->isViewFileSystem()) {
        contextMenu->addAction("to Home", this,[=]{ui->treeView->setIndexByPath(homePath);});
    }

    else if (viewMode == "processing") {
        contextMenu->addAction("Cancel operation", this, &MainWindow::cancelProcess);
        contextMenu->addAction("Cancel and Back to FileSystem View", this, [=]{emit cancelProcess(); ui->treeView->setFileSystemModel();});
    }
    else {
        contextMenu->addAction("Show FileSystem", ui->treeView, &View::setFileSystemModel);
    }

    contextMenu->addSeparator();

    if (viewMode == "folder")
        contextMenu->addAction(QString("Compute SHA-%1 for all files in folder").arg(settings["shaType"].toInt()),this,[=]{processFolderSha(path,settings["shaType"].toInt());});
    else if (viewMode == "file") {
        contextMenu->addAction("Compute SHA-1 for file",this,[=]{processFileSha(path,1);});
        contextMenu->addAction("Compute SHA-256 for file",this,[=]{processFileSha(path,256);});
        contextMenu->addAction("Compute SHA-512 for file",this,[=]{processFileSha(path,512);});
    }
    else if (viewMode == "db")
        contextMenu->addAction("Open DataBase", this, &MainWindow::doWork);
    else if (viewMode == "sum") {
        contextMenu->addAction("Check the Checksum", this, [=]{emit checkFileSummary(curPath);});
    }
    else if (viewMode == "updateMismatch")
        contextMenu->addAction("update json Database with new checksums", this, &MainWindow::updateMismatch);

    if (viewMode == "modelNewLost") {
        contextMenu->addAction("Show New/Lost only", this, &MainWindow::showNewLostOnly);
        contextMenu->addAction("Update DataBase with New/Lost files", this, &MainWindow::updateNewLost);
        contextMenu->addSeparator();
    }

    if (viewMode == "model" || viewMode == "modelNewLost") {
        contextMenu->addAction("Check current file", this, [=]{emit checkCurrentItemSum(curPath);});
        contextMenu->addAction("Check ALL files against stored checksums", this, &MainWindow::verifyFileList);
        contextMenu->addSeparator();
        contextMenu->addAction("Reset Database", this, &MainWindow::resetDatabase);
    }

    if (!ui->treeView->isViewFileSystem()) {
        contextMenu->addSeparator();
        contextMenu->addAction("Collapse all", ui->treeView, &QTreeView::collapseAll);
        contextMenu->addAction("Expand all", ui->treeView, &QTreeView::expandAll);
    }
    if (index.isValid())
        contextMenu->exec(ui->treeView->viewport()->mapToGlobal(point));

}

void MainWindow::setMode(const QString &mode)
{
    qDebug() << "MainWindow::setMode | mode:" << mode;

    if (viewMode == "processing" && mode != "endProcess")
        return;

    if (mode == "endProcess") {
        viewMode = "";
        ui->progressBar->setVisible(false);
        ui->progressBar->resetFormat();
        ui->progressBar->setValue(0);
        if (ui->treeView->isViewFileSystem()) {
            ui->treeView->pathAnalyzer(curPath);
            return;
        }
        else {
            if (previousViewMode == "model") {
                viewMode = "model";
                qDebug()<< "previousViewMode setted:" << previousViewMode;
            }
            else if (previousViewMode == "modelNewLost") {
                viewMode = "modelNewLost";
                qDebug()<< "previousViewMode setted:" << previousViewMode;
            }
        }
    }
    else {
        if (viewMode != "")
            previousViewMode = viewMode;
        viewMode = mode;
    }

    if (viewMode == "folder")
        ui->button->setText(QString("SHA-%1: Folder").arg(settings["shaType"].toInt()));
    else if (viewMode == "file")
        ui->button->setText(QString("SHA-%1: File").arg(settings["shaType"].toInt()));
    else if (viewMode == "db")
        ui->button->setText("Open DataBase");
    else if (viewMode == "sum")
        ui->button->setText("Check");
    else if (viewMode == "model")
        ui->button->setText("Verify All");
    else if (viewMode == "modelNewLost")
        ui->button->setText("Update New/Lost");
    else if (viewMode == "updateMismatch")
        ui->button->setText("Update");
    else if (viewMode == "processing") {
        ui->progressBar->setVisible(true);
        ui->button->setText("Cancel");
    }
}

void MainWindow::doWork()
{
    QString path = curPath;
    if (viewMode == "folder")
        emit processFolderSha(path, settings["shaType"].toInt());
    else if (viewMode == "file")
        emit processFileSha(path, settings["shaType"].toInt());
    else if (viewMode == "db")
        emit parseJsonFile(path);
    else if (viewMode == "sum")
        emit checkFileSummary(path);
    else if (viewMode == "model")
        emit verifyFileList();
    else if (viewMode == "modelNewLost")
        emit updateNewLost();
    else if (viewMode == "updateMismatch")
        emit updateMismatch();
    else if (viewMode == "processing")
        emit cancelProcess();
    else
        qDebug()<<"MainWindow::doWork() | Wrong viewMode:"<<viewMode;
}

void MainWindow::timeLeft(const int &percentsDone)
{
    if (percentsDone == 0) {
        elapsedTimer.start();
        return;
    }

    qint64 timePassed = elapsedTimer.elapsed();
    int leftPercents = 100 - percentsDone;

    qint64 timeleft = (timePassed / percentsDone) * leftPercents;
    int seconds = timeleft / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;
    int hours = minutes / 60;
    minutes = minutes % 60;

    QString estimatedTime;
    if (hours > 0)
        estimatedTime = QString("%1 h %2 min").arg(hours).arg(minutes);
    else if (minutes > 0 && seconds > 10)
        estimatedTime = QString("%1 min").arg(minutes + 1);
    else if (minutes > 0)
        estimatedTime = QString("%1 min").arg(minutes);
    else if (seconds > 4)
        estimatedTime = QString("%1 sec").arg(seconds);
    else
        estimatedTime = QString("few sec");

    ui->progressBar->setFormat(QString("%p% | %1").arg(estimatedTime));
}

void MainWindow::setSettings()
{
    settings["shaType"] = 256;
    settings["filterDbFiles"] = true;
    settings["filterShaFiles"] = true;
    emit settingsChanged(settings);
}

bool MainWindow::argumentInput()
{
    if (QApplication::arguments().size() > 1) {
        QString argPath = QApplication::arguments().at(1);
        qDebug()<<"App Argument:"<<argPath;
        if (QFileInfo::exists(argPath)) {
            if (QFileInfo(argPath).isFile() && argPath.endsWith(".ver.json")) {
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

void MainWindow::showMessage(const QString &message, const QString &title)
{
    QMessageBox messageBox;
    messageBox.setFixedSize(500,200);

    if(title.toLower()=="error" || title.toLower()=="failed")
        messageBox.critical(0,title,message);
    else if(title.toLower()=="warning")
        messageBox.warning(0,title,message);
    else
        messageBox.information(0,title,message);
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
        if (viewMode == "processing")
            emit cancelProcess();

        if (path.endsWith(".ver.json"))
            emit parseJsonFile(path);

        else {
            if (!ui->treeView->isViewFileSystem())
                ui->treeView->setFileSystemModel();

            ui->treeView->setIndexByPath(path);
        }
    }
}
