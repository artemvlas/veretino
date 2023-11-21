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
    bool isViewFileSystem(); // "true" if treeView's model is "*fs(QFileSystemModel)" or "false" if not
    //void pathAnalyzer(const QString &path);

    DataContainer *data_ = nullptr;
    QItemSelectionModel *oldSelectionModel_ = nullptr;

    QString curPathFileSystem;
    QString curPathModel;

    QModelIndex curIndexFileSystem;
    QModelIndex curIndexSource;
    QModelIndex curIndexProxy;


public slots:
    void setFileSystemModel();
    void setData(DataContainer *data = nullptr, ModelSelect modelSel = ModelSelect::ModelProxy);
    void setTreeModel(ModelSelect modelSel = ModelSelect::ModelProxy); // proxy = true -->> set data_->proxyModel_, else set data_->model_
    void setIndexByPath(const QString &path);
    void setFilter(const QSet<FileStatus> status);
    void toHome();

private:   
    //void saveLastPath();
    void changeCurIndexAndPath(const QModelIndex &curIndex);
    void deleteOldSelModel();
    void connectModel();
    //void sendPathChanged(const QModelIndex &index);
    QFileSystemModel *fileSystem = new QFileSystemModel;

    QString lastFileSystemPath;
    QString lastModelPath;
    void keyPressEvent(QKeyEvent* event) override;

signals:
    //void indexChanged(const QModelIndex &index);
    void pathChanged(const QString &path); // by indexToPath()
    //void setMode(Mode::Modes mode);
    void modelChanged(const bool isFileSystem); // send signal when Model has been changed, FileSystem = true, else = false;
    void dataSetted();
    void showMessage(const QString &text, const QString &title = "Info");
    void keyEnterPressed();
};

#endif // VIEW_H
