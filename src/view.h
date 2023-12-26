// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef VIEW_H
#define VIEW_H

#include <QTreeView>
#include <QFileSystemModel>
#include "tools.h"
#include <QSortFilterProxyModel>
#include "treemodel.h"
#include "proxymodel.h"
#include <QSet>
#include "datacontainer.h"

class View : public QTreeView
{
    Q_OBJECT
public:
    explicit View(QWidget *parent = nullptr);
    bool isViewFileSystem(); // "true" if this->model() is *fileSystem(QFileSystemModel), else "false"

    enum ModelView {NotSetted, FileSystem, ModelSource, ModelProxy};
    Q_ENUM(ModelView)

    ModelView currentViewModel();
    bool isCurrentViewModel(const ModelView modelView);

    DataContainer *data_ = nullptr;
    QItemSelectionModel *oldSelectionModel_ = nullptr;

    QString curPathFileSystem;
    QString curPathModel;

    QModelIndex curIndexFileSystem;
    QModelIndex curIndexSource;
    QModelIndex curIndexProxy;

public slots:
    void setFileSystemModel();
    void setData(DataContainer *data = nullptr, ModelView modelSel = ModelProxy);
    void setTreeModel(ModelView modelSel = ModelProxy);
    void setIndexByPath(const QString &path);
    void setFilter(const FileStatus status);
    void setFilter(const QSet<FileStatus> statuses = QSet<FileStatus>());
    void disableFilter();
    void toHome();

    void headerContextMenuRequested(const QPoint &point);

private:
    void changeCurIndexAndPath(const QModelIndex &curIndex);
    void deleteOldSelModel();
    void connectModel();
    void toggleColumnVisibility(int column);
    void showAllColumns();

    QFileSystemModel *fileSystem = new QFileSystemModel;

    QString lastFileSystemPath;
    QString lastModelPath;
    void keyPressEvent(QKeyEvent* event) override;

signals:
    void pathChanged(const QString &path);
    void modelChanged(ModelView modelView); //(const bool isFileSystem); // send signal when Model has been changed, FileSystem = true, else = false;
    void dataSetted();
    void showDbStatus();
    void showMessage(const QString &text, const QString &title = "Info");
    void keyEnterPressed();
}; // class View

using ModelView = View::ModelView;
#endif // VIEW_H
