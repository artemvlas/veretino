/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QDragEnterEvent>
#include "manager.h"
#include "modeselector.h"
#include "clickablelabel.h"

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
    void setProgressBar(bool processing, bool visible);
    void updatePermanentStatus();
    void handlePathEdit();
    void showDbStatus();
    void showFolderContentsDialog(const QString &folderName, const QList<ExtNumSize> &extList); // view folder contents
    void showFilterCreationDialog(const QString &folderName, const QList<ExtNumSize> &extList); // the same^ dialog, but with filter creation mode enabled
    void dialogSettings();
    void dialogOpenFolder();
    void dialogOpenJson();
    void promptOpenBranch(const QString &dbFilePath);
    void showFolderCheckResult(const Numbers &result, const QString &subFolder);
    void showFileCheckResult(const QString &filePath, const FileValues &values);

private:
    void connectManager(); // connections are separated for convenience
    void connections();
    bool argumentInput(); // using the path argument if it's provided
    void saveSettings();
    void loadSettings();
    QString getDatabaseStatusSummary();
    void handlePermanentStatusClick();
    void updateStatusIcon();

    Ui::MainWindow *ui;
    Settings *settings_ = new Settings; // current app settings
    QThread *thread = new QThread;
    Manager *manager = new Manager(settings_); // Manager performs the main tasks. Works in separate thread^
    ClickableLabel *statusIconLabel = new ClickableLabel;
    ClickableLabel *statusTextLabel = new ClickableLabel;
    ClickableLabel *permanentStatus = new ClickableLabel;
    ModeSelector *modeSelect = nullptr;
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent* event) override;
};
#endif // MAINWINDOW_H
