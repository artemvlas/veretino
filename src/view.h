// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef VIEW_H
#define VIEW_H

#include <QTreeView>
#include <QFileSystemModel>

class View : public QTreeView
{
    Q_OBJECT
public:
    explicit View(QWidget *parent = nullptr);
    bool isViewFileSystem(); //"true" if treeView's model is "*fs(QFileSystemModel)" or "false" if not
    QString indexToPath(const QModelIndex &index); // build path by current index data
    QModelIndex pathToIndex(const QString &path); // find index of specified 'path'
    void pathAnalyzer(const QString &path);
    QString workDir;

public slots:
    void setFileSystemModel();
    void smartSetModel(QAbstractItemModel *model);
    void setIndexByPath(const QString &path);
    void setItemStatus(const QString &itemPath, int status);

private:
    QFileSystemModel *fileSystem = new QFileSystemModel;
    QModelIndex currentIndex;
    QString lastFileSystemPath;
    QString lastModelPath;
    void keyPressEvent(QKeyEvent* event) override;

signals:
    void indexChanged(const QModelIndex &index);
    void pathChanged(const QString &path); //by indexToPath()
    void setMode(int mode);
    void modelChanged(const bool isFileSystem); // send signal when Model has been changed, FileSystem = true, else = false;
    void showMessage(const QString &text, const QString &title = "Info");
    void fsModel_Setted(); // fileSystem Model setted
    void keyEnterPressed();
};

#endif // VIEW_H
