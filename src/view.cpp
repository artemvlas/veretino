/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "view.h"
#include <QTimer>
#include <QDebug>
#include <QKeyEvent>
#include <QHeaderView>
#include <QAction>
#include <QMenu>
#include "tools.h"
#include "treemodeliterator.h"

View::View(QWidget *parent)
    : QTreeView(parent)
{
    sortByColumn(0, Qt::AscendingOrder);
    fileSystem->setObjectName("fileSystem");
    fileSystem->setRootPath(QDir::rootPath());

    setContextMenuPolicy(Qt::CustomContextMenu);
    header()->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(header(), &QHeaderView::customContextMenuRequested, this, &View::headerContextMenuRequested);

    connect(this, &View::modelChanged, this, &View::connectModel);
    connect(this, &View::modelChanged, this, &View::deleteOldSelModel);
    connect(this, &View::modelChanged, this, &View::setBackgroundColor);
}

// called every time the model is changed
void View::connectModel()
{
    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &View::changeCurIndexAndPath);
}

void View::setSettings(Settings *settings)
{
    settings_ = settings;
    settings_->lastFsPath = &curPathFileSystem;
}

void View::setFileSystemModel()
{
    if (isViewFileSystem()) {
        setIndexByPath();
        return;
    }

    oldSelectionModel_ = selectionModel();
    saveHeaderState();

    setModel(fileSystem);

    data_ = nullptr;

    emit modelChanged(FileSystem);

    restoreHeaderState();
    setIndexByPath();

    QTimer::singleShot(100, this, &View::switchedToFs);
}

void View::setTreeModel(ModelView modelSel)
{
    if (!data_ || !(modelSel & ModelDb)) {
        setFileSystemModel();
        return;
    }

    if (isCurrentViewModel(modelSel))
        return; // if the current model remains the same

    oldSelectionModel_ = selectionModel();
    saveHeaderState();

    modelSel == ModelSource ? setModel(data_->model_) : setModel(data_->proxyModel_);

    emit modelChanged(modelSel);

    restoreHeaderState();

    setIndexByPath(curPathModel);
}

void View::setData(DataContainer *data)
{
    if (!data) {
        setFileSystemModel();
        return;
    }

    data_ = data;
    curPathFileSystem = data->metaData.databaseFilePath;

    if (data->isInCreation()) {
        setTreeModel(ModelView::ModelSource);
    }
    else {
        setTreeModel(ModelView::ModelProxy);

        /* If the process of creating data was too fast (few data),
         * then the data comes here as the DbFileState::Created.
         * So, to avoid duplicating the "Database Status" window, an additional condition is needed.
         */

        if (data->isDbFileState(DbFileState::Saved))
            emit showDbStatus();
    }

    // the newly setted data has not yet been verified and does not contain ReChecksums
    hideColumn(Column::ColumnReChecksum);

    QTimer::singleShot(100, this, &View::dataSetted);
}

// when the process is completed, return to the Proxy Model view
void View::setViewProxy()
{
    if (isCurrentViewModel(ModelView::ModelSource)) {
        setTreeModel(ModelView::ModelProxy);
    }
}

void View::setViewSource()
{
    if (isCurrentViewModel(ModelView::ModelProxy)) {
        // if proxy model filtering is enabled, starting a Big Data queuing/verification may be very slow,
        // even if switching to Source Model, so disable filtering first
        disableFilter();

        // set the Source Model for the duration of the process,
        // because the Proxy Model is not friendly with Big Data
        setTreeModel(ModelView::ModelSource);
    }
}

void View::setMismatchFiltering(const Numbers &num)
{
    if (!data_)
        return;

    if (num.contains(FileStatus::Mismatched)) {
        showAllColumns();
        setFilter(FileStatus::Mismatched);
    }
    else {
        hideColumn(Column::ColumnReChecksum);
    }
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

void View::setIndexByPath()
{
    if (isViewFileSystem()) {
        if (!curPathFileSystem.isEmpty() && !QFileInfo::exists(curPathFileSystem))
            curPathFileSystem = paths::parentFolder(curPathFileSystem);

        QFileInfo::exists(curPathFileSystem) ? setIndexByPath(curPathFileSystem) : toHome();
    }
    else if (isViewDatabase()) {
        setIndexByPath(curPathModel);
    }
}

void View::setIndexByPath(const QString &path)
{
    if (isViewFileSystem()) {
        if (QFileInfo::exists(path)) {
            QModelIndex index = fileSystem->index(path);
            expand(index);
            setCurrentIndex(index);
            QTimer::singleShot(500, this, [=]{ scrollTo(fileSystem->index(path), QAbstractItemView::PositionAtCenter); });
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

void View::setFilter(const FileStatuses flags)
{
    if (isCurrentViewModel(ModelProxy)) {
        QString prePathModel = curPathModel;
        data_->proxyModel_->setFilter(flags);
        setIndexByPath(prePathModel);
        setBackgroundColor();
    }
    else if (isCurrentViewModel(ModelSource)) {
        data_->proxyModel_->setFilter(flags);
    }
}

void View::editFilter(const FileStatuses flags, bool add)
{
    if (isCurrentViewModel(ModelProxy)) {
        FileStatuses curFilter = data_->proxyModel_->currentlyFiltered();

        setFilter(add ? (curFilter | flags) : (curFilter & ~flags));
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
    return ModelDb & currentViewModel();
}

bool View::isViewFiltered()
{
    return isCurrentViewModel(ModelView::ModelProxy)
           && data_->proxyModel_->isFilterEnabled();
}

bool View::isViewFiltered(const FileStatus status)
{
    return isCurrentViewModel(ModelView::ModelProxy)
           && (data_->proxyModel_->currentlyFiltered() & status);
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
    if (!settings_)
        return;

    if (isViewFileSystem())
        settings_->headerStateFs = header()->saveState();
    else if (isViewDatabase())
        settings_->headerStateDb = header()->saveState();
}

void View::restoreHeaderState()
{
    QByteArray headerState;

    if (settings_) {
        if (isViewFileSystem())
            headerState = settings_->headerStateFs;
        else if (isViewDatabase())
            headerState = settings_->headerStateDb;
    }

    if (!headerState.isEmpty())
        header()->restoreState(headerState);
    else {
        showAllColumns();
        setDefaultColumnsWidth();
    }
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

void View::setBackgroundColor()
{
    QString style = isViewFiltered() ? "QTreeView { background-color : grey; color : black }" : QString();

    setStyleSheet(style);
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
