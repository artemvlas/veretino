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
    void addRecentFile();

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
    void dialogSaveJson(VerJson *pUnsavedJson);
    void dialogChooseWorkDir();

    void createContextMenu_Button(const QPoint &point);

private:
    // generic
    void connections();

    // Manager connections are separated for convenience
    void connectManager();

    // using the path argument if it's provided
    bool argumentInput();
    void handleChangedModel();
    void handleButtonDbHashClick();
    void switchToFs();

    // updates the icon in the lower left corner
    void updateStatusIcon();

    // sets the Button icon and text according the current Mode
    void updateButtonInfo();

    // updates the state of main menu actions
    void updateMenuActions();

    // closes open dialog boxes, if any
    void clearDialogs();

    // saves current settings to the file
    void saveSettings();

    Ui::MainWindow *ui;

    // current app settings
    Settings *m_settings = new Settings(this);

    // Manager thread
    QThread *m_thread = new QThread;

    // Manager performs the main tasks. Works in separate m_thread^
    Manager *m_manager = new Manager(m_settings);
    ModeSelector *m_modeSelect = nullptr;
    StatusBar *m_statusBar = new StatusBar;
    ProcState *m_proc = nullptr;

    // true if the exit attempt was rejected (to perform data saving)
    // bool awaiting_closure = false;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
}; // class MainWindow

#endif // MAINWINDOW_H
