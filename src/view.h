// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef VIEW_H
#define VIEW_H

#include <QTreeView>
#include <QFileSystemModel>
#include "tools.h"
#include <QSortFilterProxyModel>
#include "treemodel.h"
#include "proxymodel.h"

class View : public QTreeView
{
    Q_OBJECT
public:
    explicit View(QWidget *parent = nullptr);
    bool isViewFileSystem(); // "true" if treeView's model is "*fs(QFileSystemModel)" or "false" if not
    void pathAnalyzer(const QString &path);
    QString workDir;

    ProxyModel *proxyModel_ = nullptr;
    QItemSelectionModel *oldSelectionModel_ = nullptr;

public slots:
    void setFileSystemModel();
    void setTreeModel(ProxyModel *model);
    void setIndexByPath(const QString &path);
    void setFilter(const QList<int> status);

private:
    void saveLastPath();
    void deleteOldModels();
    void connectModel();
    void sendPathChanged(const QModelIndex &index);
    QFileSystemModel *fileSystem = new QFileSystemModel;
    QModelIndex currentIndex;
    QString lastFileSystemPath;
    QString lastModelPath;
    void keyPressEvent(QKeyEvent* event) override;

signals:
    void indexChanged(const QModelIndex &index);
    void pathChanged(const QString &path); // by indexToPath()
    void setMode(Mode::Modes mode);
    void modelChanged(const bool isFileSystem); // send signal when Model has been changed, FileSystem = true, else = false; init. the clearing old data
    void showMessage(const QString &text, const QString &title = "Info");
    void keyEnterPressed();
};

#endif // VIEW_H
