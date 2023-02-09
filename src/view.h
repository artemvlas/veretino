#ifndef VIEW_H
#define VIEW_H

#include <QTreeView>
#include "QFileSystemModel"

class View : public QTreeView
{
    Q_OBJECT
public:
    explicit View(QWidget *parent = nullptr);
    bool isViewFileSystem(); //"true" if treeView's model is "*fs(QFileSystemModel)" or "false" if not
    QString indexToPath(const QModelIndex &index); //build path by current index data
    void pathAnalyzer(const QString &path);

public slots:
    void setFileSystemModel();
    void smartSetModel(QAbstractItemModel *model);
    void setIndexByPath(const QString &path);

private:
    QFileSystemModel *fileSystem;
    QModelIndex currentIndex;
    QString lastFileSystemPath;
    void scrollToPath(const QString &path); //QTimer used. QFileSystemModel needs some time after setup to Scrolling be able

signals:
    void indexChanged(const QModelIndex &index);
    void pathChanged(const QString &path); //by indexToPath()
    void setMode(const QString &mode); //"file", "folder", "db", "sum"
    void modelChanged();
    void showMessage(const QString &text, const QString &title = "Info");
};

#endif // VIEW_H
