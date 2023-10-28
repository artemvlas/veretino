// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "view.h"
#include <QTimer>
#include <QDebug>
#include <QKeyEvent>
#include "treemodeliterator.h"

View::View(QWidget *parent)
    : QTreeView(parent)
{
    this->sortByColumn(0, Qt::AscendingOrder);

    fileSystem->setObjectName("fileSystem");
    fileSystem->setRootPath(QDir::rootPath());

    connect(this, &View::pathChanged, this, &View::pathAnalyzer);
}

void View::setFileSystemModel()
{
    if (isViewFileSystem()) {
        setIndexByPath(fileSystem->filePath(currentIndex));
        return;
    }

    saveLastPath();
    //deleteOldModel();

    this->setModel(fileSystem);
    emit modelChanged(true);

    connect(this->selectionModel(), &QItemSelectionModel::currentChanged, this, &View::indexChanged);
    connect(this->selectionModel(), &QItemSelectionModel::currentChanged, this, [=](const QModelIndex &index)
                                      {currentIndex = index; emit pathChanged(fileSystem->filePath(index));});

    if (!QFileInfo::exists(lastFileSystemPath))
        lastFileSystemPath = QDir::homePath();

    this->setColumnWidth(0, 450);
    this->setColumnWidth(1, 100);
    this->setIndexByPath(lastFileSystemPath);

}

void View::setTreeModel(TreeModel *model)
{
    if (model == nullptr) {
        setFileSystemModel();
        return;
    }

    saveLastPath();
    proxyModel_->setSourceModel(model);
    //deleteOldModel();

    this->setModel(proxyModel_);
    emit modelChanged(false);

    connect(this->selectionModel(), &QItemSelectionModel::currentChanged, this, &View::indexChanged);
    connect(this->selectionModel(), &QItemSelectionModel::currentChanged, this, [=](const QModelIndex &index)
                                            {currentIndex = index; emit pathChanged(paths::getPath(index));});

    this->setColumnWidth(0, 450);
    this->setColumnWidth(1, 130);
    setIndexByPath(lastModelPath);
}

bool View::isViewFileSystem()
{
    return (this->model() == fileSystem);
}

void View::setIndexByPath(const QString &path)
{
    if (isViewFileSystem()) {
        if (QFileInfo::exists(path)) {
            QModelIndex index = fileSystem->index(path);
            this->expand(index);
            this->setCurrentIndex(index);
            QTimer::singleShot(500, this, [=]{scrollTo(fileSystem->index(path), QAbstractItemView::PositionAtCenter);});
            // for better scrolling work, a timer^ is used
            // QFileSystemModel needs some time after setup to Scrolling be able
            // this is weird, but the Scrolling works well with the Timer, and only when specified [fileSystem->index(path)],
            // 'index' (wich is =fileSystem->index(path)) is NOT working good
        }
        else
            emit showMessage(QString("Wrong path: %1").arg(path), "Error");
    }
    else if (proxyModel_ != nullptr) {
        QModelIndex index = paths::getIndex(path, proxyModel_);
        if (!index.isValid())
            index = TreeModelIterator(proxyModel_).nextFile(); // select the very first file
        if (index.isValid()) {
            this->expand(index);
            this->setCurrentIndex(index);
            this->scrollTo(index, QAbstractItemView::PositionAtCenter);
        }
    }
}

void View::pathAnalyzer(const QString &path)
{
    if (isViewFileSystem()) {
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

void View::saveLastPath()
{
    if (isViewFileSystem())
        lastFileSystemPath = fileSystem->filePath(currentIndex);
    else if (proxyModel_ != nullptr && currentIndex.isValid())
        lastModelPath = paths::getPath(currentIndex);
}

void View::deleteOldModel()
{
    if (this->model() != nullptr && this->model() != fileSystem) {
        qDebug() << "'oldModel' will be deleted...";
        QAbstractItemModel *oldModel = this->model();
        delete oldModel;
        oldModel = nullptr;
    }
    if (this->selectionModel() != nullptr) {
        qDebug() << "'oldSelectionModel' will be deleted...";
        QItemSelectionModel *oldSelectionModel = this->selectionModel();
        delete oldSelectionModel;
        oldSelectionModel = nullptr;
    }
}

void View::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (this->isExpanded(currentIndex))
            this->collapse(currentIndex);
        else
            this->expand(currentIndex);
        emit keyEnterPressed();
    }

    QTreeView::keyPressEvent(event);
}
