/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/

#ifndef MENUACTIONS_H
#define MENUACTIONS_H

#include <QObject>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include "iconprovider.h"
#include "datacontainer.h"
#include "settings.h"

class MenuActions : public QObject
{
    Q_OBJECT
public:
    explicit MenuActions(QObject *parent = nullptr);
    ~MenuActions();

    void setIconTheme(const QPalette &palette);
    void setSettings(const Settings *settings);
    void setShortcuts();
    void updateMenuOpenRecent();
    void updateMenuOpenRecent(const QStringList &recentFiles);
    void populateMenuFile(QMenu *menuFile);
    QMenu* menuUpdateDb(const Numbers &dataNum); // returns *menuUpdateDatabase
    QMenu* menuAlgorithm(QCryptographicHash::Algorithm curAlgo);
    QMenu* disposableMenu() const; // (context) menu, which will be deleted when closed
    QMenu* contextMenuViewNot();

    // MainWindow menu
    QAction *actionOpenFolder = new QAction("Open Folder", this);
    QAction *actionOpenDatabaseFile = new QAction("Open Database", this);
    QAction *actionOpenDialogSettings = new QAction("Settings", this);
    QAction *actionSave = new QAction("Save", this);
    QAction *actionShowFilesystem = new QAction("Show file system", this);
    QAction *actionClearRecent = new QAction("Clear History", this);

    QList<QAction*> menuFileActions { actionOpenFolder, actionOpenDatabaseFile, actionSave, actionShowFilesystem, actionOpenDialogSettings };

    // File system View
    QAction *actionToHome = new QAction("to Home", this);
    QAction *actionCancel = new QAction("Cancel", this);
    QAction *actionShowFolderContentsTypes = new QAction("Folder Contents", this);
    QAction *actionProcessChecksumsNoFilter = new QAction("Calculate checksums [All Files]", this);
    QAction *actionProcessChecksumsPermFilter = new QAction("Calculate checksums [Permanent Filter]", this);
    QAction *actionProcessChecksumsCustomFilter = new QAction("Calculate checksums", this);
    QAction *actionCheckFileByClipboardChecksum = new QAction("Check the file by checksum: ", this);
    QAction *actionProcessSha_toClipboard = new QAction("Calculate checksum → Clipboard", this);
    QAction *actionProcessSha1File = new QAction("SHA-1 → *.sha1", this);
    QAction *actionProcessSha256File = new QAction("SHA-256 → *.sha256", this);
    QAction *actionProcessSha512File = new QAction("SHA-512 → *.sha512", this);
    QAction *actionOpenDatabase = new QAction("Open Database", this);
    QAction *actionCheckSumFile = new QAction("Check the Checksum", this);

    QList<QAction*> actionsMakeSummaries { actionProcessSha1File, actionProcessSha256File, actionProcessSha512File };

    // DB Model View
    QAction *actionCancelBackToFS = new QAction("Close the Database", this);
    QAction *actionShowDbStatus = new QAction("Status", this);
    QAction *actionResetDb = new QAction("Reset", this);
    QAction *actionForgetChanges = new QAction("Forget all changes", this);
    QAction *actionUpdateDbWithReChecksums = new QAction("Update Mismatched checksums", this);
    QAction *actionUpdateDbWithNewLost = new QAction("Add New && Clear Lost", this);
    QAction *actionDbAddNew = new QAction("Add New files", this);
    QAction *actionDbClearLost = new QAction("Clear Lost files", this);
    QAction *actionFilterNewLost = new QAction("Filter New/Lost", this);
    QAction *actionFilterMismatches = new QAction("Filter Mismatches", this);
    QAction *actionShowAll = new QAction("Show All", this);
    QAction *actionCheckCurFileFromModel = new QAction("Check File", this);
    QAction *actionCheckCurSubfolderFromModel = new QAction("Check Folder", this);
    QAction *actionBranchMake = new QAction("Branch Folder", this);
    QAction *actionBranchOpen = new QAction("Open the Branch", this);
    QAction *actionCheckAll = new QAction("Check ALL available files", this);
    QAction *actionCopyStoredChecksum = new QAction("Copy stored Checksum", this);
    QAction *actionCopyReChecksum = new QAction("Copy ReChecksum", this);
    QAction *actionCopyItem = new QAction("Copy", this);

    QAction *actionUpdFileAdd = new QAction("Add to DB", this);
    QAction *actionUpdFileRemove = new QAction("Remove from DB", this);
    QAction *actionUpdFileReChecksum = new QAction("Update Checksum", this);

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
    QMenu *menuOpenRecent = new QMenu("Open Recent");
    QMenu *menuUpdateDatabase = nullptr;
    QList<QMenu*> listOfMenus = { menuAlgo, menuStoreSummary, menuOpenRecent, menuUpdateDatabase };

private:
    void setActionsIcons();
    IconProvider iconProvider;
    const Settings *settings_ = nullptr;

}; // class MenuActions

#endif // MENUACTIONS_H
