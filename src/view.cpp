#include "view.h"
#include <QTimer>
#include <QDebug>

View::View(QWidget *parent):QTreeView(parent)
{
    connect(this, &View::pathChanged, this, &View::pathAnalyzer);
}

void View::setFileSystemModel()
{
    fileSystem = new QFileSystemModel;
    fileSystem->setObjectName("fileSystem");
    fileSystem->setRootPath(QDir::rootPath());

    this->smartSetModel(fileSystem);

    if (!QFileInfo::exists(lastFileSystemPath)) {
        lastFileSystemPath = QDir::homePath();
    }

    this->setIndexByPath(lastFileSystemPath);
    emit fsModel_Setted();
    //qDebug()<<"View::setFileSystemModel() | "<<lastFileSystemPath;
}

void View::smartSetModel(QAbstractItemModel *model)
{
    if (model == nullptr)
        return;

    if (isViewFileSystem())
        lastFileSystemPath = indexToPath(currentIndex);

    int previousColumWidth = this->columnWidth(0); // first load = 0

    QAbstractItemModel *oldModel = this->model();
    QItemSelectionModel *oldSelectModel = this->selectionModel();   

    this->setModel(model);
    qDebug() << "View::smartSetModel |" << model->objectName();

    if (previousColumWidth == 0)
        this->setColumnWidth(0, 450);

    if (oldModel != nullptr) {
        qDebug() << "oldModel deleted";
        delete oldModel;
    }
    if (oldSelectModel != nullptr) {
        qDebug() << "oldSelectModel deleted";
        delete oldSelectModel;
    }

    connect(this->selectionModel(), &QItemSelectionModel::currentChanged, this, &View::indexChanged);
    connect(this->selectionModel(), &QItemSelectionModel::currentChanged, this, [=](const QModelIndex &index){emit pathChanged(indexToPath(index)); currentIndex = index;});

    // send signal when Model has been changed, FileSystem = true, else = false;
    if (isViewFileSystem()) {
        emit modelChanged(true);
    }
    else {
        emit modelChanged(false);
        this->expandAll();
        this->scrollToTop();
    }
}

bool View::isViewFileSystem()
{
    return (this->model() != nullptr && this->model()->objectName() == "fileSystem");
}

QString View::indexToPath(const QModelIndex &index)
{
    if (isViewFileSystem()) {
        return fileSystem->filePath(index);
    }
    else if (this->model()->objectName() == "treeModel") {
        QModelIndex newIndex = this->model()->index(index.row(), 0 , index.parent());
        QString path = newIndex.data().toString();

        while (newIndex.parent().isValid()) {
            path = newIndex.parent().data().toString() + '/' + path;
            newIndex = newIndex.parent();
        }
        return path;
    }

    return QString();
}

void View::setIndexByPath(const QString &path)
{
    if (isViewFileSystem()) {

        if (QFileInfo::exists(path)) {
            QModelIndex index = fileSystem->index(path);
            this->expand(index);
            this->setCurrentIndex(index);
            this->scrollToPath(path);
            //if the Model remains the same, the Timer is not necessary, but if the Model is resetting, it does
            //this->scrollTo(index,QAbstractItemView::PositionAtCenter);
        }
        else
            emit showMessage(QString("Wrong path: %1").arg(path), "Error");
    }
}

void View::scrollToPath(const QString &path)
{
    //QFileSystemModel needs some time after setup to Scrolling be able
    //this is weird, but the Scrolling works well with the Timer, and only when specified [fileSystem->index(path)], currentIndex=fileSystem->index(path) NOT working good
    if (isViewFileSystem()) {
        QTimer::singleShot(500, this, [=]{scrollTo(fileSystem->index(path),QAbstractItemView::PositionAtCenter);});
    }
}

void View::pathAnalyzer(const QString &path)
{
    qDebug()<< "View::pathAnalyzer | " << path;

    if (this->isViewFileSystem()) {
        QFileInfo i(path);
        QString ext = i.suffix().toLower();
        if (i.isFile()) {
            if (ext == "json" && path.endsWith(".ver.json", Qt::CaseInsensitive)) {
                emit setMode("db");
            }
            else if (ext == "sha1" || ext == "sha256" || ext == "sha512") {
                emit setMode("sum");
            }
            else {
                emit setMode("file");
            }
        }
        else if (i.isDir()) {
            emit setMode("folder");
        }
    }
}
