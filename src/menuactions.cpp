/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/

#include "menuactions.h"
#include "tools.h"
#include <QFileInfo>

MenuActions::MenuActions(QObject *parent)
    : QObject{parent}
{
    actionFilterNewLost->setCheckable(true);
    actionFilterMismatches->setCheckable(true);

    actionSetAlgoSha1->setCheckable(true);
    actionSetAlgoSha256->setCheckable(true);
    actionSetAlgoSha512->setCheckable(true);

    actionGroupSelectAlgo->addAction(actionSetAlgoSha1);
    actionGroupSelectAlgo->addAction(actionSetAlgoSha256);
    actionGroupSelectAlgo->addAction(actionSetAlgoSha512);

    menuAlgo->addActions(actionGroupSelectAlgo->actions());
    menuStoreSummary->addActions(actionsMakeSummaries);

    menuOpenRecent->setToolTipsVisible(true);
}

MenuActions::~MenuActions()
{
    qDeleteAll(listOfMenus);
}

void MenuActions::setIconTheme(const QPalette &palette)
{
    iconProvider.setTheme(palette);
    setActionsIcons();
}

void MenuActions::setActionsIcons()
{
    // MainWindow menu
    actionOpenFolder->setIcon(iconProvider.icon(Icons::Folder));
    actionOpenDatabaseFile->setIcon(iconProvider.icon(Icons::Database));
    actionSave->setIcon(iconProvider.icon(Icons::Save));
    actionShowFilesystem->setIcon(iconProvider.icon(Icons::FileSystem));
    actionOpenDialogSettings->setIcon(iconProvider.icon(Icons::Configure));

    menuOpenRecent->menuAction()->setIcon(iconProvider.icon(Icons::Clock));
    actionClearRecent->setIcon(iconProvider.icon(Icons::ClearHistory));

    // File system View
    actionToHome->setIcon(iconProvider.icon(Icons::GoHome));
    actionCancel->setIcon(iconProvider.icon(Icons::Cancel));
    actionShowFolderContentsTypes->setIcon(iconProvider.icon(Icons::ChartPie));
    actionProcessChecksumsNoFilter->setIcon(iconProvider.icon(Icons::Folder));
    actionProcessChecksumsPermFilter->setIcon(iconProvider.icon(Icons::Filter));
    actionProcessChecksumsCustomFilter->setIcon(iconProvider.icon(Icons::FolderSync));
    actionCheckFileByClipboardChecksum->setIcon(iconProvider.icon(Icons::Paste));
    actionProcessSha_toClipboard->setIcon(iconProvider.icon(Icons::Copy));
    actionOpenDatabase->setIcon(iconProvider.icon(Icons::Database));
    actionCheckSumFile->setIcon(iconProvider.icon(Icons::Scan));

    menuAlgo->menuAction()->setIcon(iconProvider.icon(Icons::DoubleGear));
    menuStoreSummary->menuAction()->setIcon(iconProvider.icon(Icons::Save));

    // DB Model View
    actionCancelBackToFS->setIcon(iconProvider.icon(Icons::ProcessStop));
    actionShowDbStatus->setIcon(iconProvider.icon(Icons::Database));
    actionResetDb->setIcon(iconProvider.icon(Icons::Undo));
    actionForgetChanges->setIcon(iconProvider.icon(Icons::Backup));
    actionUpdateDbWithReChecksums->setIcon(iconProvider.icon(FileStatus::Updated));
    actionUpdateDbWithNewLost->setIcon(iconProvider.icon(Icons::Update));
    actionDbAddNew->setIcon(iconProvider.icon(FileStatus::Added));
    actionDbClearLost->setIcon(iconProvider.icon(FileStatus::Removed));
    actionFilterNewLost->setIcon(iconProvider.icon(Icons::NewFile));
    actionFilterMismatches->setIcon(iconProvider.icon(Icons::DocClose));
    actionCheckCurFileFromModel->setIcon(iconProvider.icon(Icons::Scan));
    actionCheckCurSubfolderFromModel->setIcon(iconProvider.icon(Icons::FolderSync));
    actionCheckAll->setIcon(iconProvider.icon(Icons::Start));
    actionCopyStoredChecksum->setIcon(iconProvider.icon(Icons::Copy));
    actionCopyReChecksum->setIcon(iconProvider.icon(Icons::Copy));
    actionCopyItem->setIcon(iconProvider.icon(Icons::Copy));
    actionBranchMake->setIcon(iconProvider.icon(Icons::AddFork));
    actionBranchOpen->setIcon(iconProvider.icon(Icons::Branch));
    actionUpdFileAdd->setIcon(iconProvider.icon(FileStatus::Added));
    actionUpdFileRemove->setIcon(iconProvider.icon(FileStatus::Removed));
    actionUpdFileReChecksum->setIcon(iconProvider.icon(FileStatus::Updated));
}

void MenuActions::populateMenuFile(QMenu *menuFile)
{
    menuFile->addActions(menuFileActions);
    menuFile->insertSeparator(actionOpenDialogSettings);
    menuFile->insertMenu(actionSave, menuOpenRecent);
}

void MenuActions::updateMenuOpenRecent(const QStringList &recentFiles)
{
    menuOpenRecent->clear();
    menuOpenRecent->setDisabled(recentFiles.isEmpty());

    if (!menuOpenRecent->isEnabled())
        return;

    QIcon dbIcon = iconProvider.icon(Icons::Database);

    foreach (const QString &recentFilePath, recentFiles) {
        if (QFileInfo::exists(recentFilePath)) {
            QAction *act = menuOpenRecent->addAction(dbIcon, paths::basicName(recentFilePath));
            act->setToolTip(recentFilePath);
            // connect(act, &QAction::triggered, this, [=]{ openJsonDatabase(recentFilePath); });
        }
    }

    menuOpenRecent->addSeparator();
    menuOpenRecent->addAction(actionClearRecent);
}

QMenu* MenuActions::menuUpdateDb(const Numbers &dataNum)
{
    if (!menuUpdateDatabase) {
        menuUpdateDatabase = new QMenu("Update the Database");
        menuUpdateDatabase->menuAction()->setIcon(iconProvider.icon(Icons::Update));

        menuUpdateDatabase->addAction(actionDbAddNew);
        menuUpdateDatabase->addAction(actionDbClearLost);
        menuUpdateDatabase->addAction(actionUpdateDbWithNewLost);
        menuUpdateDatabase->addSeparator();
        menuUpdateDatabase->addAction(actionUpdateDbWithReChecksums);
    }

    actionDbAddNew->setEnabled(dataNum.contains(FileStatus::New));
    actionDbClearLost->setEnabled(dataNum.contains(FileStatus::Missing));
    actionUpdateDbWithNewLost->setEnabled(actionDbAddNew->isEnabled() || actionDbClearLost->isEnabled());
    actionUpdateDbWithReChecksums->setEnabled(dataNum.contains(FileStatus::Mismatched));

    return menuUpdateDatabase;
}

// returns QMenu *menuAlgo: sets checked one of the nested actions,
// changes the text of the menu action, and returns a pointer to that menu
QMenu* MenuActions::menuAlgorithm(QCryptographicHash::Algorithm curAlgo)
{
    switch (curAlgo) {
    case QCryptographicHash::Sha1:
        actionSetAlgoSha1->setChecked(true);
        break;
    case QCryptographicHash::Sha256:
        actionSetAlgoSha256->setChecked(true);
        break;
    case QCryptographicHash::Sha512:
        actionSetAlgoSha512->setChecked(true);
        break;
    default:
        actionSetAlgoSha256->setChecked(true);
        break;
    }

    menuAlgo->menuAction()->setText("Algorithm " + format::algoToStr(curAlgo));

    return menuAlgo;
}

QMenu* MenuActions::disposableMenu() const
{
    QMenu *dispMenu = new QMenu;
    connect(dispMenu, &QMenu::aboutToHide, dispMenu, &QMenu::deleteLater);

    return dispMenu;
}

QMenu* MenuActions::contextMenuViewNot()
{
    QMenu *menuViewNot = disposableMenu();
    menuViewNot->addAction(actionShowFilesystem);

    return menuViewNot;
}
