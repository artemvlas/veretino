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
#include "pathstr.h"
#include "treemodeliterator.h"

View::View(QWidget *parent)
    : QTreeView(parent)
{
    sortByColumn(0, Qt::AscendingOrder);
    m_fileSystem->setObjectName("fileSystem");
    m_fileSystem->setRootPath(QDir::rootPath());

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
    m_settings = settings;
    m_settings->pLastFsPath = &m_lastPathFS;
}

QModelIndex View::curIndex() const
{
    if (!selectionModel()) {
        qDebug() << "View::curIndex | No selection model";
        return QModelIndex();
    }

    const QModelIndex ind = selectionModel()->currentIndex();

    if (isViewModel(ModelProxy))
        return m_data->m_proxy->mapToSource(ind);

    return ind;
}

void View::setFileSystemModel()
{
    if (isViewFileSystem()) {
        setIndexByPath();
        return;
    }

    m_oldSelectionModel = selectionModel();
    saveHeaderState();

    setModel(m_fileSystem);

    m_data = nullptr;

    emit modelChanged(FileSystem);

    restoreHeaderState();
    setIndexByPath();

    QTimer::singleShot(100, this, &View::switchedToFs);
}

void View::setTreeModel(ModelView modelSel)
{
    if (!m_data || !(modelSel & ModelDb)) {
        setFileSystemModel();
        return;
    }

    if (isViewModel(modelSel))
        return; // if the current model remains the same

    m_oldSelectionModel = selectionModel();
    saveHeaderState();

    if (modelSel == ModelSource) {
        setModel(m_data->m_model);
    } else {
        setModel(m_data->m_proxy);
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

    if (data == m_data) {
        qDebug() << "View::setData >> SKIP: this data is already setted";
        return;
    }

    m_data = data;
    m_lastPathFS = data->m_metadata.dbFilePath;

    if (DataHelper::isInCreation(data)) {
        setTreeModel(ModelView::ModelSource);
    } else {
        setTreeModel(ModelView::ModelProxy);
    }

    // the newly setted data has not yet been verified and does not contain ReChecksums
    hideColumn(Column::ColumnReChecksum);

    QTimer::singleShot(100, this, &View::dataSetted);
}

void View::clear()
{
    m_oldSelectionModel = selectionModel();
    // ? saveHeaderState();

    setModel(nullptr);
    m_data = nullptr;

    emit modelChanged(NotSet);
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
    if (!m_data)
        return;

    if (num.contains(FileStatus::Mismatched)) {
        showAllColumns();
        setFilter(FileStatus::Mismatched);
    } else {
        hideColumn(Column::ColumnReChecksum);
    }
}

QString View::curAbsPath() const
{
    if (isViewFileSystem())
        return m_fileSystem->filePath(curIndex());

    if (isViewDatabase())
        return DataHelper::itemAbsolutePath(m_data, curIndex());

    return QString();
}

void View::changeCurPath(const QModelIndex &curIndex)
{
    if (isViewFileSystem()) {
        m_lastPathFS = curAbsPath();
        emit pathChanged(m_lastPathFS);
    }
    else if (isViewDatabase()) {
        m_lastPathModel = TreeModel::getPath(curIndex);
        emit pathChanged(m_lastPathModel);
    }
    else {
        qDebug() << "View::changeCurPath | FAILURE";
    }
}

void View::setIndexByPath()
{
    if (isViewFileSystem()) {
        if (!m_lastPathFS.isEmpty() && !QFileInfo::exists(m_lastPathFS))
            m_lastPathFS = pathstr::parentFolder(m_lastPathFS);

        QFileInfo::exists(m_lastPathFS) ? setIndexByPath(m_lastPathFS) : toHome();
    }
    else if (isViewDatabase()) {
        setIndexByPath(m_lastPathModel);
    }
}

void View::setIndexByPath(const QString &path)
{
    if (isViewFileSystem()) {
        if (QFileInfo::exists(path)) {
            setCurIndex(m_fileSystem->index(path));

            //OLD
            /*QModelIndex index = m_fileSystem->index(path);
            expand(index);
            setCurrentIndex(index);
            QTimer::singleShot(500, this, [=]{ scrollTo(m_fileSystem->index(path), QAbstractItemView::PositionAtCenter); });
            // for better scrolling work, a timer^ is used
            // QFileSystemModel needs some time after setup to Scrolling be able
            // this is weird, but the Scrolling works well with the Timer, and only when specified [m_fileSystem->index(path)],
            // 'index' (wich is == m_fileSystem->index(path)) is NOT working good*/
        } else if (!path.isEmpty()) {
            emit showMessage("Wrong path: " + path, "Error");
        }
    } else if (isViewDatabase()) {
        QModelIndex ind = TreeModel::getIndex(path, model());

        if (!ind.isValid())
            ind = TreeModelIterator(model()).nextFile().index(); // select the very first file

        setCurIndex(ind);
    }
}

void View::setCurIndex(const QModelIndex &ind)
{
    if (ind.isValid()) {
        expand(ind);
        setCurrentIndex(ind);
        const int tmr = (ind.model() == m_fileSystem) ? 500 : 0;
        QTimer::singleShot(tmr, this, &View::scrollToCurrent);

        if (tmr) // second, the control one :)
            QTimer::singleShot((tmr * 2), this, &View::scrollToCurrent);
    }
}

void View::scrollToCurrent()
{
    scrollTo(currentIndex(), QAbstractItemView::PositionAtCenter);
}

void View::setFilter(const FileStatuses flags)
{
    if (isViewModel(ModelProxy)) {
        QString prePathModel = m_lastPathModel;
        m_data->m_proxy->setFilter(flags);
        setIndexByPath(prePathModel);
        setBackgroundColor();
    }
    else if (isViewModel(ModelSource)) {
        m_data->m_proxy->setFilter(flags);
    }
}

void View::editFilter(const FileStatuses flags, bool add)
{
    if (isViewModel(ModelProxy)) {
        FileStatuses curFilter = m_data->m_proxy->currentlyFiltered();

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
        m_lastPathFS = QDir::homePath();
        setIndexByPath(m_lastPathFS);
    }
}

ModelView View::curViewModel() const
{
    if (model() == m_fileSystem)
        return FileSystem;
    else if (m_data && model() == m_data->m_model)
        return ModelSource;
    else if (m_data && model() == m_data->m_proxy)
        return ModelProxy;
    else
        return NotSet;
}

bool View::isViewModel(const ModelView modelView) const
{
    return (modelView == curViewModel());
}

bool View::isViewFileSystem() const
{
    return (model() == m_fileSystem);
}

bool View::isViewDatabase() const
{
    return (ModelDb & curViewModel());
}

bool View::isViewFiltered() const
{
    return isViewModel(ModelView::ModelProxy)
           && m_data->m_proxy->isFilterEnabled();
}

bool View::isViewFiltered(const FileStatus status) const
{
    return isViewModel(ModelView::ModelProxy)
           && (m_data->m_proxy->currentlyFiltered() & status);
}

void View::deleteOldSelModel()
{
    if (m_oldSelectionModel && (m_oldSelectionModel != selectionModel())) {
        //qDebug() << "'oldSelectionModel' will be deleted...";
        delete m_oldSelectionModel;
        m_oldSelectionModel = nullptr;
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
    if (!m_settings)
        return;

    if (isViewFileSystem())
        m_settings->m_headerStateFs = header()->saveState();
    else if (isViewDatabase())
        m_settings->m_headerStateDb = header()->saveState();
}

void View::restoreHeaderState()
{
    QByteArray headerState;

    if (m_settings) {
        if (isViewFileSystem())
            headerState = m_settings->m_headerStateFs;
        else if (isViewDatabase())
            headerState = m_settings->m_headerStateDb;
    }

    if (!headerState.isEmpty()) {
        header()->restoreState(headerState);
    } else {
        showAllColumns();
        setDefaultColumnsWidth();
    }
}

void View::headerContextMenuRequested(const QPoint &point)
{
    if (isViewModel(NotSet))
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
