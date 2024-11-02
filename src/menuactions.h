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
#include "settings.h"
#include "numbers.h"

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
    QAction *actionChooseFolder = new QAction(QStringLiteral(u"Choose Folder..."), this);
    QAction *actionOpenDatabaseFile = new QAction(QStringLiteral(u"Open Database..."), this);
    QAction *actionOpenDialogSettings = new QAction(QStringLiteral(u"Settings..."), this);
    QAction *actionSave = new QAction(QStringLiteral(u"Save"), this);
    QAction *actionShowFilesystem = new QAction(QStringLiteral(u"Show file system"), this);
    QAction *actionClearRecent = new QAction(QStringLiteral(u"Clear History"), this);
    QAction *actionAbout = new QAction(QStringLiteral(u"About"), this);

    QList<QAction*> menuFileActions { actionChooseFolder, actionOpenDatabaseFile, actionSave, actionShowFilesystem, actionOpenDialogSettings };

    // File system View
    QAction *actionToHome = new QAction(QStringLiteral(u"to Home"), this);
    QAction *actionStop = new QAction(QStringLiteral(u"Stop"), this);
    QAction *actionShowFolderContentsTypes = new QAction(QStringLiteral(u"Folder Contents"), this);
    QAction *actionProcessChecksumsNoFilter = new QAction(QStringLiteral(u"Calculate checksums [All Files]"), this);
    QAction *actionProcessChecksumsCustomFilter = new QAction(QStringLiteral(u"Calculate checksums"), this);
    QAction *actionCheckFileByClipboardChecksum = new QAction(QStringLiteral(u"Check the file by checksum: "), this);
    QAction *actionProcessSha_toClipboard = new QAction(QStringLiteral(u"Copy Checksum"), this);
    QAction *actionProcessSha1File = new QAction(QStringLiteral(u"SHA-1 → *.sha1"), this);
    QAction *actionProcessSha256File = new QAction(QStringLiteral(u"SHA-256 → *.sha256"), this);
    QAction *actionProcessSha512File = new QAction(QStringLiteral(u"SHA-512 → *.sha512"), this);
    QAction *actionOpenDatabase = new QAction(QStringLiteral(u"Open Database"), this);
    QAction *actionCheckSumFile = new QAction(QStringLiteral(u"Check the Checksum"), this);

    QList<QAction*> actionsMakeDigest { actionProcessSha1File, actionProcessSha256File, actionProcessSha512File };

    // DB Model View
    QAction *actionCancelBackToFS = new QAction(QStringLiteral(u"Close the Database"), this);
    QAction *actionShowDbStatus = new QAction(QStringLiteral(u"Status"), this);
    QAction *actionResetDb = new QAction(QStringLiteral(u"Reset"), this);
    QAction *actionForgetChanges = new QAction(QStringLiteral(u"Forget all changes"), this);
    QAction *actionFilterNewLost = new QAction(QStringLiteral(u"Filter New/Lost"), this);
    QAction *actionFilterMismatches = new QAction(QStringLiteral(u"Filter Mismatches"), this);
    QAction *actionFilterUnreadable = new QAction(QStringLiteral(u"Filter Unreadable"), this);
    QAction *actionFilterModified = new QAction(QStringLiteral(u"Filter Modified"), this);
    QAction *actionShowAll = new QAction(QStringLiteral(u"Show All"), this);
    QAction *actionCheckCurFileFromModel = new QAction(QStringLiteral(u"Check File"), this);
    QAction *actionCheckCurSubfolderFromModel = new QAction(QStringLiteral(u"Check Folder"), this);
    QAction *actionBranchMake = new QAction(QStringLiteral(u"Branch Folder"), this);
    QAction *actionBranchOpen = new QAction(QStringLiteral(u"Open Branch"), this);
    QAction *actionCheckAll = new QAction(QStringLiteral(u"Check ALL available files"), this);
    QAction *actionCheckAllMod = new QAction(QStringLiteral(u"Check Modified files"), this);
    QAction *actionCopyStoredChecksum = new QAction(QStringLiteral(u"Copy stored Checksum"), this);
    QAction *actionCopyReChecksum = new QAction(QStringLiteral(u"Copy ReChecksum"), this);
    QAction *actionExportSum = new QAction(QStringLiteral(u"Export to *.sha"), this);

    QAction *actionUpdDbAddNew = new QAction(QStringLiteral(u"Add New files"), this);
    QAction *actionUpdDbClearLost = new QAction(QStringLiteral(u"Clear Lost files"), this);
    QAction *actionUpdDbNewLost = new QAction(QStringLiteral(u"Add New && Clear Lost"), this);
    QAction *actionUpdDbReChecksums = new QAction(QStringLiteral(u"Update Mismatched checksums"), this);
    QAction *actionUpdDbFindMoved = new QAction(QStringLiteral(u"Find Moved"), this);

    QAction *actionUpdFileAdd = new QAction(QStringLiteral(u"Add to DB"), this);
    QAction *actionUpdFileRemove = new QAction(QStringLiteral(u"Remove from DB"), this);
    QAction *actionUpdFileReChecksum = new QAction(QStringLiteral(u"Update Checksum"), this);
    QAction *actionUpdFileImportDigest = new QAction(QStringLiteral(u"Import Digest (*.sha)"), this);

    QAction *actionCollapseAll = new QAction(QStringLiteral(u"Collapse all"), this);
    QAction *actionExpandAll = new QAction(QStringLiteral(u"Expand all"), this);

    QAction *actionCopyFile = new QAction(QStringLiteral(u"Copy File"), this);
    QAction *actionCopyFolder = new QAction(QStringLiteral(u"Copy Folder"), this);

    // Algorithm selection
    QAction *actionSetAlgoSha1 = new QAction(QStringLiteral(u"SHA-1"), this);
    QAction *actionSetAlgoSha256 = new QAction(QStringLiteral(u"SHA-256"), this);
    QAction *actionSetAlgoSha512 = new QAction(QStringLiteral(u"SHA-512"), this);
    QActionGroup *actionGroupSelectAlgo = new QActionGroup(this);

    // Menu
    QMenu *menuAlgo = new QMenu;
    QMenu *menuCreateDigest = new QMenu(QStringLiteral(u"Create Digest file"));
    QMenu *menuOpenRecent = new QMenu(QStringLiteral(u"Open Recent"));
    QMenu *menuUpdateDatabase = nullptr;
    QList<QMenu*> listOfMenus = { menuAlgo, menuCreateDigest, menuOpenRecent, menuUpdateDatabase };

private:
    void setActionsIcons();
    IconProvider _icons;
    const Settings *settings_ = nullptr;

}; // class MenuActions

#endif // MENUACTIONS_H
