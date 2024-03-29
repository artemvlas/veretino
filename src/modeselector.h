/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef MODESELECTOR_H
#define MODESELECTOR_H

#include <QObject>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include "view.h"
#include "iconprovider.h"
#include "manager.h"

class ModeSelector : public QObject
{
    Q_OBJECT
public:
    explicit ModeSelector(View *view, QPushButton *button, Settings *settings, QObject *parent = nullptr);

    enum Mode {
        NoMode,
        Folder,
        File,
        DbFile,
        SumFile,
        Model,
        ModelNewLost,
        UpdateMismatch,
    };
    Q_ENUM(Mode)

    // modes
    void setMode();
    Mode currentMode();
    bool isCurrentMode(const Mode mode);
    bool isProcessing();
    void setButtonInfo(); // sets the Button icon and text according the current Mode
    void setAlgorithm(QCryptographicHash::Algorithm algo);
    //QSet<Mode> fsModes {Folder, File, DbFile, SumFile};
    //QSet<Mode> dbModes {Model, ModelNewLost, UpdateMismatch};

    // tasks execution
    void quickAction();
    void doWork();
    void processFolderChecksums();
    void processFolderChecksums(const FilterRule &filter);
    void processFolderFilteredChecksums();
    void openFsPath(const QString &path);
    void openJsonDatabase(const QString &filePath);
    bool processAbortPrompt(const bool sendCancelation = true); // abort process if any and allow further execution (true)
                                                                // or do not abort the process, cancel execution (false)

    //---->>>
    void procSumFile(QCryptographicHash::Algorithm algo);
    void verifyItem();
    void verifyDb();
    void showFolderContentTypes();
    void checkFileChecksum(const QString &checkSum);

    void showFileSystem();
    void updateMenuOpenRecent();
    QMenu* menuAlgorithm(); // QMenu *menuAlgo: sets checked one of the nested actions, changes the text of the menu action, and returns a pointer to that menu

    IconProvider iconProvider;

    // Actions --->>
    // MainWindow menu
    QAction *actionOpenFolder = new QAction("Open Folder", this);
    QAction *actionOpenDatabaseFile = new QAction("Open Database", this);
    QAction *actionOpenSettingsDialog = new QAction("Settings", this);
    QAction *actionShowFilesystem = new QAction("Show file system", this);

    QList<QAction*> menuFileActions {actionOpenFolder, actionOpenDatabaseFile, actionShowFilesystem, actionOpenSettingsDialog};

    QMenu *menuOpenRecent = new QMenu("Open Recent");
    QAction *actionClearRecent = new QAction("Clear History");

    // File system View    
    QAction *actionToHome = new QAction("to Home", this);
    QAction *actionCancel = new QAction("Cancel", this);
    QAction *actionShowFolderContentsTypes = new QAction("Folder Contents", this);
    QAction *actionProcessContainedChecksums = new QAction("Calculate checksums of Contained files", this);
    QAction *actionProcessFilteredChecksums = new QAction("Calculate checksums of Filtered files", this);
    QAction *actionCheckFileByClipboardChecksum = new QAction("Check the file by checksum: ", this);
    QAction *actionProcessSha_toClipboard = new QAction("Calculate checksum → Clipboard", this);
    QAction *actionProcessSha1File = new QAction("SHA-1 → *.sha1", this);
    QAction *actionProcessSha256File = new QAction("SHA-256 → *.sha256", this);
    QAction *actionProcessSha512File = new QAction("SHA-512 → *.sha512", this);
    QAction *actionOpenDatabase = new QAction("Open Database", this);
    QAction *actionCheckSumFile = new QAction("Check the Checksum", this);

    QList<QAction*> actionsMakeSummaries {actionProcessSha1File, actionProcessSha256File, actionProcessSha512File};

    // DB Model View
    QAction *actionCancelBackToFS = new QAction("Close the Database", this);
    QAction *actionShowDbStatus = new QAction("Status", this);
    QAction *actionResetDb = new QAction("Reset", this);
    QAction *actionForgetChanges = new QAction("Forget all changes", this);
    QAction *actionUpdateDbWithReChecksums = new QAction("Update the Database with new checksums", this);
    QAction *actionUpdateDbWithNewLost = new QAction("Update the Database with New/Lost files", this);
    QAction *actionShowNewLostOnly = new QAction("Show only New/Lost", this);
    QAction *actionShowMismatchesOnly = new QAction("Show only Mismatches", this);
    QAction *actionShowAll = new QAction("Show All", this);
    QAction *actionCheckCurFileFromModel = new QAction("Check the File", this);
    QAction *actionCheckCurSubfolderFromModel = new QAction("Check the Subfolder", this);
    QAction *actionBranchSubfolder = new QAction("Branch the Subfolder", this);
    QAction *actionCheckAll = new QAction("Check ALL available files", this);
    QAction *actionCopyStoredChecksum = new QAction("Copy stored Checksum", this);
    QAction *actionCopyReChecksum = new QAction("Copy ReChecksum", this);

    QAction *actionCollapseAll = new QAction("Collapse all", this);
    QAction *actionExpandAll = new QAction("Expand all", this);

    // Algorithm selection
    QAction *actionSetAlgoSha1 = new QAction("SHA-1", this);
    QAction *actionSetAlgoSha256 = new QAction("SHA-256", this);
    QAction *actionSetAlgoSha512 = new QAction("SHA-512", this);
    QActionGroup *actionGroupSelectAlgo = new QActionGroup(this);

    // Menu
    QMenu *menuAlgo = new QMenu;
    QMenu *menuStoreSummary = new QMenu("Calculate checksum → Summary");

public slots:
    void processing(bool isProcessing);
    void prepareView();
    void createContextMenu_View(const QPoint &point);
    void createContextMenu_Button(const QPoint &point);

private:
    void connectActions();
    Mode selectMode(const Numbers &numbers); // select Mode based on the contents of the Numbers struct
    Mode selectMode(const QString &path); // select Mode based on file system path

    void copyDataToClipboard(Column column);
    void setActionsIcons();

    Mode curMode = NoMode;
    bool isProcessing_ = false;
    View *view_;
    QPushButton *button_;
    Settings *settings_;

signals:
    void getPathInfo(const QString &path); // info about folder contents or file (size)
    void getIndexInfo(const QModelIndex &curIndex); // info about database item (file or subfolder index)
    void processFolderSha(const MetaData &metaData);
    void processFileSha(const QString &path, QCryptographicHash::Algorithm algo, ProcFileResult result = ProcFileResult::Generic);
    void parseJsonFile(const QString &path);
    void verify(const QModelIndex& index = QModelIndex());
    void updateNewLost();
    void updateMismatch(); // update json Database with new checksums for files with failed verification
    void checkSummaryFile(const QString &path);
    void checkFile(const QString &filePath, const QString &checkSum);
    void cancelProcess();
    void resetDatabase(); // reopening and reparsing current database
    void restoreDatabase();
    void dbItemContents(const QString &itemPath);
    void makeFolderContentsList(const QString &folderPath);
    void makeFolderContentsFilter(const QString &folderPath);
    void branchSubfolder(const QModelIndex &subfolder);
    void makeSumFile(const QString &originFilePath, const QString &checksum);
}; // class ModeSelector

using Mode = ModeSelector::Mode;
#endif // MODESELECTOR_H
