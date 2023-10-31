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

    connect(this, &View::pathChanged, this, &View::pathAnalyzer);
    connect(this, &View::modelChanged, this, &View::connectModel);
    connect(this, &View::modelChanged, this, &View::deleteOldModels);
}

// called every time the model is changed
void View::connectModel()
{
    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &View::indexChanged);
    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &View::sendPathChanged);
}

void View::setFileSystemModel()
{
    if (isViewFileSystem()) {
        setIndexByPath(fileSystem->filePath(currentIndex));
        return;
    }

    saveLastPath();
    oldSelectionModel_ = selectionModel();

    setModel(fileSystem);
    emit modelChanged(true);

    if (!QFileInfo::exists(lastFileSystemPath))
        lastFileSystemPath = QDir::homePath();

    setColumnWidth(0, 450);
    setColumnWidth(1, 100);
    setIndexByPath(lastFileSystemPath);
}

void View::setTreeModel(ProxyModel *model)
{
    if (!model) {
        setFileSystemModel();
        return;
    }

    saveLastPath();
    oldSelectionModel_ = selectionModel();
    proxyModel_ = model;

    setModel(proxyModel_);
    emit modelChanged(false);

    setColumnWidth(0, 450);
    setColumnWidth(1, 100);
    setIndexByPath(lastModelPath);
}

void View::sendPathChanged(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    currentIndex = index;

    if (isViewFileSystem())
        emit pathChanged(fileSystem->filePath(index));
    else if (proxyModel_)
        emit pathChanged(paths::getPath(index));
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
    else if (proxyModel_) {
        QModelIndex index = paths::getIndex(path, proxyModel_);
        if (!index.isValid())
            index = TreeModelIterator(proxyModel_).nextFile().index(); // select the very first file
        if (index.isValid()) {
            expand(index);
            setCurrentIndex(index);
            scrollTo(index, QAbstractItemView::PositionAtCenter);
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

void View::setFilter(const QList<int> status)
{
    if (proxyModel_)
        proxyModel_->setFilter(status);
}

void View::saveLastPath()
{
    if (isViewFileSystem())
        lastFileSystemPath = fileSystem->filePath(currentIndex);
    else if (currentIndex.isValid())
        lastModelPath = paths::getPath(proxyModel_->mapFromSource(currentIndex));
}

void View::deleteOldModels()
{
    if (oldSelectionModel_) {
        qDebug() << "'oldSelectionModel' will be deleted...";
        delete oldSelectionModel_;
        oldSelectionModel_ = nullptr;
    }
}

void View::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (isExpanded(currentIndex))
            collapse(currentIndex);
        else
            expand(currentIndex);
        emit keyEnterPressed();
    }

    QTreeView::keyPressEvent(event);
}
