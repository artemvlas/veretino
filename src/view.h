/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef VIEW_H
#define VIEW_H

#include <QTreeView>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include "datacontainer.h"
#include "settings.h"

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

    void setSettings(Settings *settings);
    QString curAbsPath() const;
    ModelView curViewModel() const;
    bool isViewModel(const ModelView modelView) const;

    // "true" if this->model() is *m_fileSystem(QFileSystemModel), else "false"
    bool isViewFileSystem() const;
    bool isViewDatabase() const;
    bool isViewFiltered() const;
    bool isViewFiltered(const FileStatus status) const;
    QModelIndex curIndex() const;

    DataContainer *m_data = nullptr;
    QItemSelectionModel *m_oldSelectionModel = nullptr;

    QString m_lastPathFS;
    QString m_lastPathModel;

public slots:
    void setFileSystemModel();
    void setData(DataContainer *data);
    void setTreeModel(ModelView modelSel = ModelProxy);
    void clear();
    void setIndexByPath();
    void setIndexByPath(const QString &path);
    void setMismatchFiltering(const Numbers &num);
    void setFilter(const FileStatuses flags = FileStatus::NotSet);
    void editFilter(const FileStatuses flags, bool add);
    void disableFilter();
    void saveHeaderState();
    void toHome();

    void setViewSource();
    void setViewProxy();

    void headerContextMenuRequested(const QPoint &point);

private:
    QString headerText(int column) const;
    void changeCurPath(const QModelIndex &curIndex);
    void deleteOldSelModel();
    void connectModel();
    void setBackgroundColor();
    void toggleColumnVisibility(int column);
    void showAllColumns();
    void setDefaultColumnsWidth();
    void restoreHeaderState();
    void setCurIndex(const QModelIndex &ind);
    void scrollToCurrent();

    QFileSystemModel *m_fileSystem = new QFileSystemModel(this);
    Settings *m_settings = nullptr;

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
