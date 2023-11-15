// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
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
#include <QCryptographicHash>
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
    void setMode(Mode::Modes mode); // sets viewMode and button text
    void showMessage(const QString &message, const QString &title = "Info");
    void onCustomContextMenu(const QPoint &point);

private:
    void connectManager(); // connections with Manager separated for convenience
    void connections();
    void doWork();
    void quickAction(); // tasks for some items when double-clicking or pressing Enter
    bool argumentInput(); // using the path argument if it's provided
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    void closeEvent(QCloseEvent *event);
    bool processAbortPrompt();
    void keyPressEvent(QKeyEvent* event) override;

    Ui::MainWindow *ui;
    Settings *settings_ = new Settings; // stores the app settings
    QThread *thread = new QThread;
    Manager *manager = new Manager(settings_); // Manager performs the main tasks. Works in separate thread^
    QLabel *permanentStatus = new QLabel;
    QString homePath = QDir::homePath();   
    QString curPath; // current path from &View::pathChanged
    Mode::Modes viewMode = Mode::NoMode; // Folder, File, DbFile, SumFile, Model...
    Mode::Modes previousViewMode = Mode::NoMode; //^

signals:
    void getItemInfo(const QString &path); // get file size or folder contents info
    void processFolderSha(const QString &path, QCryptographicHash::Algorithm algo);
    void processFileSha(const QString &path, QCryptographicHash::Algorithm algo, bool summaryFile = true, bool clipboard = false);
    void parseJsonFile(const QString &path);
    void verify(const QModelIndex& index = QModelIndex());
    void updateNewLost();
    void updateMismatch(); // update json Database with new checksums for files with failed verification
    void checkSummaryFile(const QString &path);
    void checkFile(const QString &filePath, const QString &checkSum);
    void copyStoredChecksum(const QModelIndex& fileItemIndex);
    void cancelProcess();
    void resetDatabase(); // reopening and reparsing current database
    void restoreDatabase();
    void dbItemContents(const QString &itemPath);
    void folderContentsByType(const QString &folderPath);
    void dbStatus();
};
#endif // MAINWINDOW_H
