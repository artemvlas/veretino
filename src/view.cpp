#include "view.h"
#include "tools.h"
#include <QTimer>
#include <QDebug>

View::View(QWidget *parent)
    : QTreeView(parent)
{
    fileSystem->setObjectName("fileSystem");
    fileSystem->setRootPath(QDir::rootPath());

    connect(this, &View::pathChanged, this, &View::pathAnalyzer);
}

void View::setFileSystemModel()
{
    this->smartSetModel(fileSystem);

    if (!QFileInfo::exists(lastFileSystemPath)) {
        lastFileSystemPath = QDir::homePath();
    }

    this->setIndexByPath(lastFileSystemPath);
    emit fsModel_Setted();
}

void View::smartSetModel(QAbstractItemModel *model)
{
    if (model == nullptr) {
        setFileSystemModel();
        return;
    }

    if (isViewFileSystem()) {
        lastFileSystemPath = indexToPath(currentIndex);
        if (model->objectName() == "fileSystem")
            return;
    }
    else if (currentIndex.isValid())
        lastModelPath = indexToPath(currentIndex);

    int previousColumWidth = this->columnWidth(0); // first load = 0

    QAbstractItemModel *oldModel = this->model();
    QItemSelectionModel *oldSelectModel = this->selectionModel();   

    this->setModel(model);
    qDebug() << "View::smartSetModel |" << model->objectName();

    if (previousColumWidth == 0)
        this->setColumnWidth(0, 450);

    if (oldModel != nullptr && oldModel->objectName() != "fileSystem") {
        qDebug() << "oldModel deleted";
        delete oldModel;
    }
    if (oldSelectModel != nullptr) {
        qDebug() << "oldSelectModel deleted";
        delete oldSelectModel;
    }

    connect(this->selectionModel(), &QItemSelectionModel::currentChanged, this, &View::indexChanged);
    connect(this->selectionModel(), &QItemSelectionModel::currentChanged, this, [=](const QModelIndex &index)
                                               {emit pathChanged(indexToPath(index)); currentIndex = index;});

    // send signal when Model has been changed, FileSystem = true, else = false;
    if (isViewFileSystem()) {
        emit modelChanged(true);
        this->setColumnWidth(1, 100);
    }
    else {
        emit modelChanged(false);
        this->setColumnWidth(1, 130);
        setIndexByPath(lastModelPath);
    }
}

bool View::isViewFileSystem()
{
    return (this->model() != nullptr && this->model()->objectName() == "fileSystem");
}

QString View::indexToPath(const QModelIndex &index)
{
    QString path;

    if (isViewFileSystem()) {
        path = fileSystem->filePath(index);
    }
    else if (this->model()->objectName() == "treeModel") {
        QModelIndex newIndex = this->model()->index(index.row(), 0 , index.parent());
        path = newIndex.data().toString();

        while (newIndex.parent().isValid()) {
            path = newIndex.parent().data().toString() + '/' + path;
            newIndex = newIndex.parent();
        }
    }

    return path;
}

QModelIndex View::pathToIndex(const QString &path)
{
    QModelIndex curIndex = this->model()->index(0, 0);
    if (path.isEmpty())
        return curIndex;

    QStringList parts = path.split('/');
    QModelIndex parentIndex;

    foreach (const QString &str, parts) {
        for (int i = 0; curIndex.isValid(); ++i) {
            curIndex = this->model()->index(i, 0, parentIndex);
            if (curIndex.data().toString() == str) {
                //qDebug() << "***" << str << "finded on" << i << "row";
                parentIndex = this->model()->index(i, 0, parentIndex);
                break;
            }
            //qDebug() << "*** Looking for:" << str << curIndex.data();
        }
    }
    //qDebug() << "View::pathToIndex" << path << "-->" << curIndex << curIndex.data();
    return curIndex;
}

void View::setIndexByPath(const QString &path)
{
    if (isViewFileSystem()) {
        if (QFileInfo::exists(path)) {
            QModelIndex index = fileSystem->index(path);
            this->expand(index);
            this->setCurrentIndex(index);
            this->scrollToPath(path);
            // for better scrolling work, a timer^ is used
            // this->scrollTo(index, QAbstractItemView::PositionAtCenter);
        }
        else
            emit showMessage(QString("Wrong path: %1").arg(path), "Error");
    }
    else {
        QModelIndex index = pathToIndex(path);
        if (index.isValid()) {
            this->expand(index);
            this->setCurrentIndex(index);
            this->scrollTo(index, QAbstractItemView::PositionAtCenter);
        }
        else {
            this->expandAll();
            this->scrollToTop();
        }
    }
}

void View::scrollToPath(const QString &path)
{
    // QFileSystemModel needs some time after setup to Scrolling be able
    // this is weird, but the Scrolling works well with the Timer, and only when specified [fileSystem->index(path)],
    // 'currentIndex' (wich is =fileSystem->index(path)) is NOT working good
    if (isViewFileSystem()) {
        QTimer::singleShot(500, this, [=]{scrollTo(fileSystem->index(path), QAbstractItemView::PositionAtCenter);});
    }
}

void View::setItemStatus(const QString &itemPath, int status)
{
    QModelIndex curIndex = pathToIndex(itemPath);
    QModelIndex chIndex = this->model()->index(curIndex.row(), 2, curIndex.parent());

    this->model()->setData(chIndex, format::fileItemStatus(status));
}

void View::pathAnalyzer(const QString &path)
{
    if (this->isViewFileSystem()) {
        QFileInfo i(path);
        if (i.isFile()) {
            if (tools::isDatabaseFile(path)) {
                emit setMode(Mode::DbFile);
            }
            else if (tools::isSummaryFile(path)) {
                emit setMode(Mode::SumFile);
            }
            else {
                emit setMode(Mode::File);
            }
        }
        else if (i.isDir()) {
            emit setMode(Mode::Folder);
        }
    }
}
