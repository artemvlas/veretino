// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "view.h"
#include <QTimer>
#include <QDebug>
#include <QKeyEvent>
#include "treemodeliterator.h"

View::View(QWidget *parent)
    : QTreeView(parent)
{
    sortByColumn(0, Qt::AscendingOrder);
    fileSystem->setObjectName("fileSystem");
    fileSystem->setRootPath(QDir::rootPath());

    connect(this, &View::modelChanged, this, &View::connectModel);
    connect(this, &View::modelChanged, this, &View::deleteOldSelModel);
}

// called every time the model is changed
void View::connectModel()
{
    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &View::changeCurIndexAndPath);
}

void View::setFileSystemModel()
{
    if (isViewFileSystem()) {
        setIndexByPath(fileSystem->filePath(curIndexFileSystem));
        return;
    }

    oldSelectionModel_ = selectionModel();

    setModel(fileSystem);

    emit modelChanged(FileSystem);

    data_ = nullptr;

    setColumnWidth(0, 450);
    setColumnWidth(1, 100);

    QFileInfo::exists(curPathFileSystem) ? setIndexByPath(curPathFileSystem) : toHome();
}

void View::setTreeModel(ModelView modelSel)
{
    if (!data_ && (modelSel != ModelSource || modelSel != ModelProxy)) {
        setFileSystemModel();
        return;
    }

    if ((modelSel == ModelSource && model() == data_->model_)
        || (modelSel == ModelProxy && model() == data_->proxyModel_))
        return; // if current model remains the same

    oldSelectionModel_ = selectionModel();

    modelSel == ModelSource ? setModel(data_->model_) : setModel(data_->proxyModel_);

    emit modelChanged(modelSel);

    setColumnWidth(0, 450);
    setColumnWidth(1, 100);
    setIndexByPath(curPathModel);

    //hideColumn(ModelKit::ColumnReChecksum);
}

void View::setData(DataContainer *data, ModelView modelSel)
{
    if (!data) {
        setFileSystemModel();
        return;
    }

    data_ = data;
    setTreeModel(modelSel);
    emit dataSetted();
}

void View::changeCurIndexAndPath(const QModelIndex &curIndex)
{
    if (isViewFileSystem()) {
        curIndexFileSystem = curIndex;
        curPathFileSystem = fileSystem->filePath(curIndex);
        emit pathChanged(curPathFileSystem);
    }
    else if (data_ && (model() == data_->model_ || model() == data_->proxyModel_)) {
        if (model() == data_->model_) {
            curIndexSource = curIndex;
            curIndexProxy = data_->proxyModel_->mapFromSource(curIndexSource);
        }
        else if (model() == data_->proxyModel_) {
            curIndexProxy = curIndex;
            curIndexSource = data_->proxyModel_->mapToSource(curIndexProxy);
        }

        curPathModel = ModelKit::getPath(curIndexSource);
        emit pathChanged(curPathModel);
    }
    else
        qDebug() << "View::changeCurIndexAndPath | FAILURE";
}

bool View::isViewFileSystem()
{
    return (model() == fileSystem);
}

void View::setIndexByPath(const QString &path)
{
    if (isViewFileSystem()) {
        if (QFileInfo::exists(path)) {
            QModelIndex index = fileSystem->index(path);
            expand(index);
            setCurrentIndex(index);
            QTimer::singleShot(500, this, [=]{scrollTo(fileSystem->index(path), QAbstractItemView::PositionAtCenter);});
            // for better scrolling work, a timer^ is used
            // QFileSystemModel needs some time after setup to Scrolling be able
            // this is weird, but the Scrolling works well with the Timer, and only when specified [fileSystem->index(path)],
            // 'index' (wich is =fileSystem->index(path)) is NOT working good
        }
        else
            emit showMessage(QString("Wrong path: %1").arg(path), "Error");
    }
    else if (data_) {
        QModelIndex index = ModelKit::getIndex(path, model());

        if (!index.isValid())
            index = TreeModelIterator(model()).nextFile().index(); // select the very first file

        if (index.isValid()) {
            expand(index);
            setCurrentIndex(index);
            scrollTo(index, QAbstractItemView::PositionAtCenter);
        }
    }
}

void View::setFilter(const QSet<FileStatus> status)
{
    if (currentViewModel() == ModelProxy) {
        data_->proxyModel_->setFilter(status);
        setIndexByPath(curPathModel);
    }
}

void View::disableFilter()
{
    setFilter();
}

void View::toHome()
{
    if (isViewFileSystem()) {
        curPathFileSystem = QDir::homePath();
        setIndexByPath(curPathFileSystem);
    }
}

ModelView View::currentViewModel()
{
    if (model() == fileSystem)
        return FileSystem;
    else if (data_ && model() == data_->model_)
        return ModelSource;
    else if (data_ && model() == data_->proxyModel_)
        return ModelProxy;
    else
        return NotSetted;
}

void View::deleteOldSelModel()
{
    if (oldSelectionModel_ && (oldSelectionModel_ != selectionModel())) {
        //qDebug() << "'oldSelectionModel' will be deleted...";
        delete oldSelectionModel_;
        oldSelectionModel_ = nullptr;
    }
}

void View::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        isExpanded(currentIndex()) ? collapse(currentIndex())
                                   : expand(currentIndex());
        emit keyEnterPressed();
    }

    QTreeView::keyPressEvent(event);
}
