#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "QThread"
#include "treemodel.h"
#include "manager.h"
#include "settingdialog.h"
#include "aboutdialog.h"
#include "QMessageBox"
#include "QFileDialog"
#include "QClipboard"
#include "QDragEnterEvent"
#include "QMimeData"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QThread *thread = new QThread;
    Manager *manager = new Manager; // Manager performs the main tasks. Works in separate thread^
    QString homePath = QDir::homePath();
    QVariantMap settings; // stores the app settings

    QString viewMode; // "folder", "file", "db", "sum", "model"...
    QString previousViewMode; //^
    QElapsedTimer elapsedTimer;
    void connectManager(); // connections with Manager separated for convenience
    void connections();
    void doWork();
    void setSettings();
    bool argumentInput(); // using the path argument if it's provided
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    void timeLeft(const int &percentsDone);

private slots:
    void onCustomContextMenu(const QPoint &point);
    void setMode(const QString &mode); // sets viewMode^ and button text
    void showMessage(const QString &message, const QString &title = "Info");

signals:
    void getFInfo(const QString &path); //get file size or folder size and number of files
    void processFolderSha(const QString &path, const int &shatype = 0);
    void processFileSha(const QString &path, const int &shatype = 0);
    void parseJsonFile(const QString &path);
    void verifyFileList();
    void updateNewLost();
    void updateMismatch();// update json Database with new checksums for files with failed verification
    void checkCurrentItemSum(const QString &path); //check selected file only instead all database cheking
    void checkFileSummary(const QString &path);
    void settingsChanged(const QVariantMap &settingsMap);
    void cancelProcess();
    void resetDatabase(); // reopening and reparsing current database
};
#endif // MAINWINDOW_H
