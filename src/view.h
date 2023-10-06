// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef VIEW_H
#define VIEW_H

#include <QTreeView>
#include <QFileSystemModel>
#include "tools.h"
#include "treemodel.h"

class View : public QTreeView
{
    Q_OBJECT
public:
    explicit View(QWidget *parent = nullptr);
    bool isViewFileSystem(); // "true" if treeView's model is "*fs(QFileSystemModel)" or "false" if not
    void pathAnalyzer(const QString &path);
    QString workDir;
    TreeModel *model_ = nullptr;

public slots:
    void setFileSystemModel();
    void setTreeModel(TreeModel *model);
    void setIndexByPath(const QString &path);

private:
    void saveLastPath();
    void deleteOldModel();
    QFileSystemModel *fileSystem = new QFileSystemModel;
    QModelIndex currentIndex;
    QString lastFileSystemPath;
    QString lastModelPath;
    void keyPressEvent(QKeyEvent* event) override;

signals:
    void indexChanged(const QModelIndex &index);
    void pathChanged(const QString &path); // by indexToPath()
    void setMode(Mode::Modes mode);
    void modelChanged(const bool isFileSystem); // send signal when Model has been changed, FileSystem = true, else = false;
    void showMessage(const QString &text, const QString &title = "Info");
    void keyEnterPressed();
};

#endif // VIEW_H
