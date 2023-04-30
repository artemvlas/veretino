#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QMessageBox>
#include <QFileDialog>
#include <QClipboard>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QLabel>
#include "treemodel.h"
#include "manager.h"
#include "settingdialog.h"
#include "aboutdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void setMode(const QString &mode); // sets viewMode^ and button text
    void showMessage(const QString &message, const QString &title = "Info");
    void onCustomContextMenu(const QPoint &point);

private:
    void connectManager(); // connections with Manager separated for convenience
    void connections();
    void doWork();
    void setSettings();
    bool argumentInput(); // using the path argument if it's provided
    void timeLeft(const int percentsDone);
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);

    Ui::MainWindow *ui;
    QThread *thread = new QThread;
    Manager *manager = new Manager; // Manager performs the main tasks. Works in separate thread^
    QLabel *permanentStatus = new QLabel;
    QString homePath = QDir::homePath();
    QVariantMap settings; // stores the app settings
    QString curPath; // current path from &View::pathChanged
    QString viewMode; // "folder", "file", "db", "sum", "model"...
    QString previousViewMode; //^
    QElapsedTimer elapsedTimer;

signals:
    void getItemInfo(const QString &path); //get file size or folder contents info
    void processFolderSha(const QString &path, int shatype = 0);
    void processFileSha(const QString &path, int shatype = 0);
    void parseJsonFile(const QString &path);
    void verifyFileList();
    void updateNewLost();
    void updateMismatch();// update json Database with new checksums for files with failed verification
    void checkCurrentItemSum(const QString &path); //check selected file only instead all database cheking
    void checkFileSummary(const QString &path);
    void copyStoredChecksum(const QString &path, bool clipboard = true);
    void settingsChanged(const QVariantMap &settingsMap);
    void cancelProcess();
    void resetDatabase(); // reopening and reparsing current database
    void showNewLostOnly();
    void dbItemContents(const QString &itemPath);
    void folderContentsByType(const QString &folderPath);
    void showAll();
    void aboutDatabase();
};
#endif // MAINWINDOW_H
