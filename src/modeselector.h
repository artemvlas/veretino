// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef MODESELECTOR_H
#define MODESELECTOR_H

#include <QObject>
#include <QAction>
#include "view.h"

class ModeSelector : public QObject
{
    Q_OBJECT
public:
    explicit ModeSelector(View *view, Settings *settings, QObject *parent = nullptr);

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
    bool isProcessing();
    //QSet<Mode> fsModes {Folder, File, DbFile, SumFile};
    //QSet<Mode> dbModes {Model, ModelNewLost, UpdateMismatch};

    // tasks execution
    void quickAction();
    void doWork();

    //---->>>
    void computeFileChecksum(QCryptographicHash::Algorithm algo, bool summaryFile = true, bool clipboard = false);
    void verifyItem();
    void verifyDb();
    void showFolderContentTypes();
    void checkFileChecksum(const QString &checkSum);

    void showFileSystem();

    // Actions
    QAction *actionShowFS = new QAction("Show FileSystem", this);
    QAction *actionToHome = new QAction("to Home", this);
    QAction *actionCancel = new QAction("Cancel operation", this);
    QAction *actionShowFolderContentsTypes = new QAction("Folder Contents By Type", this);
    QAction *actionProcessFolderChecksums = new QAction("Compute checksums for all files in folder", this);
    QAction *actionCheckFileByClipboardChecksum = new QAction("Check the file by checksum: ", this);
    QAction *actionProcessSha1File = new QAction("SHA-1 --> *.sha1", this);
    QAction *actionProcessSha256File = new QAction("SHA-256 --> *.sha256", this);
    QAction *actionProcessSha512File = new QAction("SHA-512 --> *.sha512", this);
    QAction *actionOpenDatabase = new QAction("Open Database", this);
    QAction *actionCheckSumFile = new QAction("Check the Checksum", this);

    QAction *actionCancelBackToFS = new QAction("Cancel and Back to FileSystem view", this);
    QAction *actionShowDbStatus = new QAction("Status", this);
    QAction *actionResetDb = new QAction("Reset", this);
    QAction *actionForgetChanges = new QAction("Forget all changes", this);
    QAction *actionUpdateDbWithReChecksums = new QAction("Update the Database with new checksums", this);
    QAction *actionUpdateDbWithNewLost = new QAction("Update the Database with New/Lost files", this);
    QAction *actionShowNewLostOnly = new QAction("Show New/Lost only", this);
    QAction *actionShowMismatchesOnly = new QAction("Show only Mismatches", this);
    QAction *actionShowAll = new QAction("Show All", this);
    QAction *actionCheckCurFileFromModel = new QAction("Check current file", this);
    QAction *actionCheckCurSubfolderFromModel = new QAction("Check current Subfolder", this);
    QAction *actionCheckAll = new QAction("Check ALL files against stored checksums", this);
    QAction *actionCopyStoredChecksum = new QAction("Copy stored checksum to clipboard", this);
    QAction *actionCopyReChecksum = new QAction("Copy ReChecksum to clipboard", this);

    QAction *actionCollapseAll = new QAction("Collapse all", this);
    QAction *actionExpandAll = new QAction("Expand all", this);

public slots:
    void processing(bool isProcessing);
    void prepareView();
    void createContextMenu_View(const QPoint &point);

private:
    void connectActions();
    Mode selectMode(const Numbers &numbers); // select Mode based on the contents of the Numbers struct
    Mode selectMode(const QString &path); // select Mode based on file system path

    void copyDataToClipboard(Column column);

    Mode curMode = NoMode;
    bool isProcessing_ = false;
    View *view_;
    Settings *settings_;

signals:
    // ui
    void setButtonText(const QString &buttonText);

    // tasks
    void getPathInfo(const QString &path); // info about folder contents or file (size)
    void getIndexInfo(const QModelIndex &curIndex); // info about database item (file or subfolder index)
    void processFolderSha(const QString &path, QCryptographicHash::Algorithm algo);
    void processFileSha(const QString &path, QCryptographicHash::Algorithm algo, bool summaryFile = true, bool clipboard = false);
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
    void folderContentsByType(const QString &folderPath);
    void dbStatus();
}; // class ModeSelector

using Mode = ModeSelector::Mode;
#endif // MODESELECTOR_H
