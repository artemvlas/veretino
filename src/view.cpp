/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "view.h"
#include <QTimer>
#include <QDebug>
#include <QKeyEvent>
#include "treemodeliterator.h"
#include <QHeaderView>
#include <QAction>
#include <QMenu>

View::View(QWidget *parent)
    : QTreeView(parent)
{
    sortByColumn(0, Qt::AscendingOrder);
    fileSystem->setObjectName("fileSystem");
    fileSystem->setRootPath(QDir::rootPath());

    header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(header(), &QHeaderView::customContextMenuRequested, this, &View::headerContextMenuRequested);

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
        setIndexByPath(curPathFileSystem);
        return;
    }

    oldSelectionModel_ = selectionModel();
    saveHeaderState();

    setModel(fileSystem);

    data_ = nullptr;

    emit modelChanged(FileSystem);

    if (headerStateFs.isEmpty()) {
        showAllColumns();
        setDefaultColumnsWidth();
    }
    else
        header()->restoreState(headerStateFs);

    QFileInfo::exists(curPathFileSystem) ? setIndexByPath(curPathFileSystem) : toHome();
}

void View::setTreeModel(ModelView modelSel)
{
    if (!data_ || (modelSel != ModelSource && modelSel != ModelProxy)) {
        setFileSystemModel();
        return;
    }

    if (modelSel == currentViewModel())
        return; // if the current model remains the same

    oldSelectionModel_ = selectionModel();
    saveHeaderState();

    modelSel == ModelSource ? setModel(data_->model_) : setModel(data_->proxyModel_);

    emit modelChanged(modelSel);

    if (!headerStateDb.isEmpty())
        header()->restoreState(headerStateDb);
    else
        setDefaultColumnsWidth();

    data_->numbers.numberOf(FileStatus::Mismatched) == 0 ? hideColumn(Column::ColumnReChecksum)
                                                         : showAllColumns();

    setIndexByPath(curPathModel);
}

void View::setData(DataContainer *data)
{
    if (!data) {
        setFileSystemModel();
        return;
    }

    data_ = data;
    curPathFileSystem = data->metaData.isImported ? data->metaData.databaseFilePath : data->metaData.workDir;

    if (data->metaData.isImported) {
        setTreeModel(ModelView::ModelProxy);
        emit showDbStatus();
    }
    else
        setTreeModel(ModelView::ModelSource);

    emit dataSetted();
}

void View::changeCurIndexAndPath(const QModelIndex &curIndex)
{
    if (isViewFileSystem()) {
        curIndexFileSystem = curIndex;
        curPathFileSystem = fileSystem->filePath(curIndex);
        emit pathChanged(curPathFileSystem);
    }
    else if (isViewDatabase()) {
        if (isCurrentViewModel(ModelSource)) {
            curIndexSource = curIndex;
            curIndexProxy = data_->proxyModel_->mapFromSource(curIndexSource);
        }
        else if (isCurrentViewModel(ModelProxy)) {
            curIndexProxy = curIndex;
            curIndexSource = data_->proxyModel_->mapToSource(curIndexProxy);
        }

        curPathModel = TreeModel::getPath(curIndexSource);
        emit pathChanged(curPathModel);
    }
    else
        qDebug() << "View::changeCurIndexAndPath | FAILURE";
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
        else if (!path.isEmpty())
            emit showMessage(QString("Wrong path: %1").arg(path), "Error");
    }
    else if (isViewDatabase()) {
        QModelIndex index = TreeModel::getIndex(path, model());

        if (!index.isValid())
            index = TreeModelIterator(model()).nextFile().index(); // select the very first file

        if (index.isValid()) {
            expand(index);
            setCurrentIndex(index);
            scrollTo(index, QAbstractItemView::PositionAtCenter);
        }
    }
}

void View::setFilter(const FileStatus status)
{
    setFilter(QSet<FileStatus>({status}));
}

void View::setFilter(const QSet<FileStatus> statuses)
{
    if (isCurrentViewModel(ModelProxy)) {
        QString prePathModel = curPathModel;
        data_->proxyModel_->setFilter(statuses);
        setIndexByPath(prePathModel);
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

bool View::isCurrentViewModel(const ModelView modelView)
{
    return modelView == currentViewModel();
}

bool View::isViewFileSystem()
{
    return (model() == fileSystem);
}

bool View::isViewDatabase()
{
    return isCurrentViewModel(ModelProxy) || isCurrentViewModel(ModelSource);
}

void View::deleteOldSelModel()
{
    if (oldSelectionModel_ && (oldSelectionModel_ != selectionModel())) {
        //qDebug() << "'oldSelectionModel' will be deleted...";
        delete oldSelectionModel_;
        oldSelectionModel_ = nullptr;
    }
}

QString View::headerText(int column)
{
    return model()->headerData(column, Qt::Horizontal).toString();
}

void View::toggleColumnVisibility(int column)
{
    setColumnHidden(column, !isColumnHidden(column));
}

void View::showAllColumns()
{
    if (!model())
        return;

    for (int i = 0; i < model()->columnCount(); ++i) {
        showColumn(i);
    }
}

void View::setDefaultColumnsWidth()
{
    setColumnWidth(0, 450);
    setColumnWidth(1, 100);
}

void View::saveHeaderState()
{
    if (isViewFileSystem())
        headerStateFs = header()->saveState();
    else if (isViewDatabase())
        headerStateDb = header()->saveState();
}

void View::headerContextMenuRequested(const QPoint &point)
{
    if (isCurrentViewModel(NotSetted))
        return;

    QMenu *headerContextMenu = new QMenu(this);
    connect(headerContextMenu, &QMenu::aboutToHide, headerContextMenu, &QMenu::deleteLater);

    for (int i = 1; i < model()->columnCount(); ++i) {
        QAction *curAct = new QAction(headerText(i), headerContextMenu);
        curAct->setCheckable(true);
        curAct->setChecked(!isColumnHidden(i));
        connect(curAct, &QAction::toggled, this, [=]{toggleColumnVisibility(i);});
        headerContextMenu->addAction(curAct);
    }

    headerContextMenu->addSeparator();
    headerContextMenu->addAction("Show all", this, &View::showAllColumns);
    headerContextMenu->exec(header()->mapToGlobal(point));
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
