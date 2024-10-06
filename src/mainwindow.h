/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QDragEnterEvent>
#include "manager.h"
#include "modeselector.h"
#include "statusbar.h"
#include "dialogdbstatus.h"

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
    void showMessage(const QString &message, const QString &title = "Info");
    void updatePermanentStatus();
    void handlePathEdit();
    void setWinTitleMismatchFound();
    void updateWindowTitle();

    // dialogs
    void showDbStatus();
    void showDbStatusTab(DialogDbStatus::Tabs tab);
    void showDialogContentsList(const QString &folderName, const FileTypeList &extList); // view folder contents
    void showDialogDbCreation(const QString &folder, const QStringList &dbFiles, const FileTypeList &extList);
    void showDialogDbContents(const QString &folderName, const FileTypeList &extList); // view DB contents
    void dialogSettings();
    void dialogChooseFolder();
    void dialogOpenJson();
    void promptOpenBranch(const QString &dbFilePath);
    void showFolderCheckResult(const Numbers &result, const QString &subFolder);
    void showFileCheckResult(const QString &filePath, const FileValues &values);

    void createContextMenu_Button(const QPoint &point);

private:
    void connectManager(); // connections are separated for convenience
    void connections();
    bool argumentInput(); // using the path argument if it's provided
    void handleChangedModel();
    void handleButtonDbHashClick();
    void updateStatusIcon();
    void updateButtonInfo(); // sets the Button icon and text according the current Mode
    void saveSettings();

    Ui::MainWindow *ui;
    Settings *settings_ = new Settings(this); // current app settings
    QThread *thread = new QThread;
    Manager *manager = new Manager(settings_); // Manager performs the main tasks. Works in separate thread^
    ModeSelector *modeSelect = nullptr;
    ProcState *proc_ = nullptr;
    StatusBar *statusBar = new StatusBar;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;
}; // class MainWindow

#endif // MAINWINDOW_H
