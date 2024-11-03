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
    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &View::changeCurPath);
}

void View::setSettings(Settings *settings)
{
    settings_ = settings;
    settings_->lastFsPath = &_lastPathFS;
}

QModelIndex View::curIndex() const
{
    if (!selectionModel()) {
        qDebug() << "View::curIndex | No selection model";
        return QModelIndex();
    }

    const QModelIndex _ind = selectionModel()->currentIndex();

    if (isViewModel(ModelProxy))
        return data_->proxyModel_->mapToSource(_ind);

    return _ind;
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

    if (isViewModel(modelSel))
        return; // if the current model remains the same

    oldSelectionModel_ = selectionModel();
    saveHeaderState();

    if (modelSel == ModelSource) {
        setModel(data_->model_);
    } else {
        setModel(data_->proxyModel_);
    }

    emit modelChanged(modelSel);

    restoreHeaderState();
    // setIndexByPath(curPathModel);
    QTimer::singleShot(0, this, qOverload<>(&View::setIndexByPath));
}

void View::setData(DataContainer *data)
{
    if (!data) {
        setFileSystemModel();
        return;
    }

    if (data == data_) {
        qDebug() << "View::setData >> SKIP: this data is already setted";
        return;
    }

    data_ = data;
    _lastPathFS = data->metaData_.dbFilePath;

    if (data->isInCreation()) {
        setTreeModel(ModelView::ModelSource);
    }
    else {
        setTreeModel(ModelView::ModelProxy);
    }

    // the newly setted data has not yet been verified and does not contain ReChecksums
    hideColumn(Column::ColumnReChecksum);

    QTimer::singleShot(100, this, &View::dataSetted);
}

void View::clear()
{
    oldSelectionModel_ = selectionModel();
    // ? saveHeaderState();

    setModel(nullptr);
    data_ = nullptr;

    emit modelChanged(NotSetted);
}

// when the process is completed, return to the Proxy Model view
void View::setViewProxy()
{
    if (isViewModel(ModelView::ModelSource)) {
        //setTreeModel(ModelView::ModelProxy);
        QTimer::singleShot(0, this, [=]{ setTreeModel(ModelView::ModelProxy); });
    }
}

void View::setViewSource()
{
    if (isViewModel(ModelView::ModelProxy)) {
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

QString View::curAbsPath() const
{
    if (isViewFileSystem())
        return fileSystem->filePath(curIndex());

    if (isViewDatabase())
        return data_->itemAbsolutePath(curIndex());

    return QString();
}

void View::changeCurPath(const QModelIndex &curIndex)
{
    if (isViewFileSystem()) {
        _lastPathFS = curAbsPath();
        emit pathChanged(_lastPathFS);
    }
    else if (isViewDatabase()) {
        _lastPathModel = TreeModel::getPath(curIndex);
        emit pathChanged(_lastPathModel);
    }
    else {
        qDebug() << "View::changeCurPath | FAILURE";
    }
}

void View::setIndexByPath()
{
    if (isViewFileSystem()) {
        if (!_lastPathFS.isEmpty() && !QFileInfo::exists(_lastPathFS))
            _lastPathFS = paths::parentFolder(_lastPathFS);

        QFileInfo::exists(_lastPathFS) ? setIndexByPath(_lastPathFS) : toHome();
    }
    else if (isViewDatabase()) {
        setIndexByPath(_lastPathModel);
    }
}

void View::setIndexByPath(const QString &path)
{
    if (isViewFileSystem()) {
        if (QFileInfo::exists(path)) {
            setCurIndex(fileSystem->index(path));

            //OLD
            /*QModelIndex index = fileSystem->index(path);
            expand(index);
            setCurrentIndex(index);
            QTimer::singleShot(500, this, [=]{ scrollTo(fileSystem->index(path), QAbstractItemView::PositionAtCenter); });
            // for better scrolling work, a timer^ is used
            // QFileSystemModel needs some time after setup to Scrolling be able
            // this is weird, but the Scrolling works well with the Timer, and only when specified [fileSystem->index(path)],
            // 'index' (wich is ==fileSystem->index(path)) is NOT working good*/
        }
        else if (!path.isEmpty()) {
            emit showMessage("Wrong path: " + path, "Error");
        }
    }
    else if (isViewDatabase()) {
        QModelIndex _ind = TreeModel::getIndex(path, model());

        if (!_ind.isValid())
            _ind = TreeModelIterator(model()).nextFile().index(); // select the very first file

        setCurIndex(_ind);
    }
}

void View::setCurIndex(const QModelIndex &ind)
{
    if (ind.isValid()) {
        expand(ind);
        setCurrentIndex(ind);
        const int _tmr = (ind.model() == fileSystem) ? 500 : 0;
        QTimer::singleShot(_tmr, this, &View::scrollToCurrent);

        if (_tmr) // second, the control one :)
            QTimer::singleShot((_tmr * 2), this, &View::scrollToCurrent);
    }
}

void View::scrollToCurrent()
{
    scrollTo(currentIndex(), QAbstractItemView::PositionAtCenter);
}

void View::setFilter(const FileStatuses flags)
{
    if (isViewModel(ModelProxy)) {
        QString prePathModel = _lastPathModel;
        data_->proxyModel_->setFilter(flags);
        setIndexByPath(prePathModel);
        setBackgroundColor();
    }
    else if (isViewModel(ModelSource)) {
        data_->proxyModel_->setFilter(flags);
    }
}

void View::editFilter(const FileStatuses flags, bool add)
{
    if (isViewModel(ModelProxy)) {
        FileStatuses curFilter = data_->proxyModel_->currentlyFiltered();

        setFilter(add ? (curFilter | flags) : (curFilter & ~flags));
    }
}

void View::disableFilter()
{
    if (isViewFiltered()) {
        setFilter();
    }
}

void View::toHome()
{
    if (isViewFileSystem()) {
        _lastPathFS = QDir::homePath();
        setIndexByPath(_lastPathFS);
    }
}

ModelView View::curViewModel() const
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

bool View::isViewModel(const ModelView modelView) const
{
    return (modelView == curViewModel());
}

bool View::isViewFileSystem() const
{
    return (model() == fileSystem);
}

bool View::isViewDatabase() const
{
    return (ModelDb & curViewModel());
}

bool View::isViewFiltered() const
{
    return isViewModel(ModelView::ModelProxy)
           && data_->proxyModel_->isFilterEnabled();
}

bool View::isViewFiltered(const FileStatus status) const
{
    return isViewModel(ModelView::ModelProxy)
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

QString View::headerText(int column) const
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
    if (isViewModel(NotSetted))
        return;

    QMenu *headerContextMenu = new QMenu(this);
    connect(headerContextMenu, &QMenu::aboutToHide, headerContextMenu, &QMenu::deleteLater);

    for (int i = 1; i < model()->columnCount(); ++i) {
        QAction *curAct = new QAction(headerText(i), headerContextMenu);
        curAct->setCheckable(true);
        curAct->setChecked(!isColumnHidden(i));
        connect(curAct, &QAction::toggled, this, [=]{ toggleColumnVisibility(i); });
        headerContextMenu->addAction(curAct);
    }

    headerContextMenu->addSeparator();
    headerContextMenu->addAction(QStringLiteral(u"Show all"), this, &View::showAllColumns);
    headerContextMenu->exec(header()->mapToGlobal(point));
}

void View::setBackgroundColor()
{
    QString style = isViewFiltered() ? QStringLiteral(u"QTreeView { background-color : grey; color : black }")
                                     : QString();

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
