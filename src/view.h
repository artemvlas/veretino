/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef VIEW_H
#define VIEW_H

#include <QTreeView>
#include <QFileSystemModel>
#include "tools.h"
#include <QSortFilterProxyModel>
#include "treemodel.h"
#include "proxymodel.h"
#include "datacontainer.h"

class View : public QTreeView
{
    Q_OBJECT
public:
    explicit View(QWidget *parent = nullptr);
    enum ModelView {
        NotSetted = 1,
        FileSystem = 1 << 1,
        ModelSource = 1 << 2,
        ModelProxy = 1 << 3,
        ModelDb = ModelSource | ModelProxy
    };
    Q_ENUM(ModelView)

    enum ColumnFileSystem { ColumnFsName, ColumnFsSize, ColumnFsType, ColumnFsDateModified };

    ModelView currentViewModel();
    bool isCurrentViewModel(const ModelView modelView);
    bool isViewFileSystem(); // "true" if this->model() is *fileSystem(QFileSystemModel), else "false"
    bool isViewDatabase();
    bool isViewFiltered();
    bool isViewFiltered(const FileStatus status);

    DataContainer *data_ = nullptr;
    QItemSelectionModel *oldSelectionModel_ = nullptr;

    QString curPathFileSystem;
    QString curPathModel;

    QModelIndex curIndexFileSystem;
    QModelIndex curIndexSource;
    QModelIndex curIndexProxy;

    QByteArray headerStateFs;
    QByteArray headerStateDb;

public slots:
    void setFileSystemModel();
    void setData(DataContainer *data);
    void setTreeModel(ModelView modelSel = ModelProxy);
    void setIndexByPath(const QString &path);
    void setFilter(const FileStatuses flags = FileStatus::NotSet);
    void editFilter(const FileStatuses flags, bool add);
    void disableFilter();
    void saveHeaderState();
    void toHome();
    void setMismatchFiltering(const Numbers &num);

    void setViewSource();
    void setViewProxy();

    void headerContextMenuRequested(const QPoint &point);

private:
    void changeCurIndexAndPath(const QModelIndex &curIndex);
    void deleteOldSelModel();
    void connectModel();
    void setBackgroundColor();
    void toggleColumnVisibility(int column);
    void showAllColumns();
    void setDefaultColumnsWidth();
    QString headerText(int column);

    QFileSystemModel *fileSystem = new QFileSystemModel(this);

protected:
    void keyPressEvent(QKeyEvent* event) override;

signals:
    void pathChanged(const QString &path);
    void modelChanged(ModelView modelView);
    void dataSetted();
    void switchedToFs();
    void showDbStatus();
    void showMessage(const QString &text, const QString &title = "Info");
    void keyEnterPressed();
}; // class View

using ModelView = View::ModelView;
#endif // VIEW_H
