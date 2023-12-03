// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QMessageBox>
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QLabel>
#include <QCryptographicHash>
#include "treemodel.h"
#include "manager.h"
#include "settingdialog.h"
#include "aboutdialog.h"
#include "modeselector.h"

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

    void dialogSettings();
    void dialogOpenFolder();
    void dialogOpenJson();

private:
    void connectManager(); // connections with Manager separated for convenience
    void connections();
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
    ModeSelector *modeSelect = nullptr;
};
#endif // MAINWINDOW_H
