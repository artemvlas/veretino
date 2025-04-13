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
    void showMessage(const QString &message,
                     const QString &title = "Info");
    void updatePermanentStatus();
    void handlePathEdit();
    void setWinTitleMismatchFound();
    void updateWindowTitle();

    // dialogs
    void showDbStatus();
    void showDbStatusTab(DialogDbStatus::Tabs tab);

    // view folder contents
    void showDialogContentsList(const QString &folderName,
                                const FileTypeList &extList);

    void showDialogDbCreation(const QString &folder,
                              const QStringList &dbFiles,
                              const FileTypeList &extList);

    // view DB contents
    void showDialogDbContents(const QString &folderName,
                              const FileTypeList &extList);
    void dialogSettings();
    void dialogChooseFolder();
    void dialogOpenJson();
    void promptOpenBranch(const QString &dbFilePath);
    void showFolderCheckResult(const Numbers &result,
                               const QString &subFolder);
    void showFileCheckResult(const QString &filePath,
                             const FileValues &values);
    void dialogSaveJson(VerJson *p_unsaved);

    void createContextMenu_Button(const QPoint &point);

private:
    void connections();                                        // generic
    void connectManager();                                     // Manager connections are separated for convenience
    bool argumentInput();                                      // using the path argument if it's provided
    void handleChangedModel();
    void handleButtonDbHashClick();
    void updateStatusIcon();                                   // updates the icon in the lower left corner
    void updateButtonInfo();                                   // sets the Button icon and text according the current Mode
    void updateMenuActions();                                  // updates the state of main menu actions
    void clearDialogs();                                       // closes open dialog boxes, if any
    void saveSettings();                                       // saves current settings to the file

    Ui::MainWindow *ui;
    Settings *settings_ = new Settings(this);                  // current app settings
    QThread *thread = new QThread;                             // Manager thread
    Manager *manager = new Manager(settings_);                 // Manager performs the main tasks. Works in separate thread^
    ModeSelector *modeSelect = nullptr;
    ProcState *proc_ = nullptr;
    StatusBar *statusBar = new StatusBar;
    bool awaiting_closure = false;                             // true if the exit attempt was rejected (to perform data saving)

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
}; // class MainWindow

#endif // MAINWINDOW_H
