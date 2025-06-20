/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/

#include "menuactions.h"
#include "tools.h"
#include "pathstr.h"
#include <QFileInfo>

MenuActions::MenuActions(QObject *parent)
    : QObject{parent}
{
    actionFilterNewLost->setCheckable(true);
    actionFilterMismatches->setCheckable(true);
    actionFilterUnreadable->setCheckable(true);
    actionFilterModified->setCheckable(true);

    actionSetAlgoSha1->setCheckable(true);
    actionSetAlgoSha256->setCheckable(true);
    actionSetAlgoSha512->setCheckable(true);

    actionGroupSelectAlgo->addAction(actionSetAlgoSha1);
    actionGroupSelectAlgo->addAction(actionSetAlgoSha256);
    actionGroupSelectAlgo->addAction(actionSetAlgoSha512);

    menuAlgo->addActions(actionGroupSelectAlgo->actions());
    menuCreateDigest->addActions(m_actionsMakeDigest);

    menuOpenRecent->setToolTipsVisible(true);

    setShortcuts();

    actionSave->setEnabled(false);
}

MenuActions::~MenuActions()
{
    qDeleteAll(m_listOfMenus);
}

void MenuActions::setIconTheme(const QPalette &palette)
{
    m_icons.setTheme(palette);
    setActionsIcons();
}

void MenuActions::setSettings(const Settings *settings)
{
    m_settings = settings;
}

void MenuActions::setActionsIcons()
{
    // MainWindow menu
    actionChooseFolder->setIcon(m_icons.icon(Icons::Folder));
    actionOpenDatabaseFile->setIcon(m_icons.icon(Icons::Database));
    actionSave->setIcon(m_icons.icon(Icons::Save));
    actionShowFilesystem->setIcon(m_icons.icon(Icons::FileSystem));
    actionOpenDialogSettings->setIcon(m_icons.icon(Icons::Configure));

    menuOpenRecent->menuAction()->setIcon(m_icons.icon(Icons::Clock));
    actionClearRecent->setIcon(m_icons.icon(Icons::ClearHistory));
    actionAbout->setIcon(m_icons.icon(Icons::Info));

    // File system View
    actionToHome->setIcon(m_icons.icon(Icons::GoHome));
    actionStop->setIcon(m_icons.icon(Icons::ProcessStop));
    actionShowFolderContentsTypes->setIcon(m_icons.icon(Icons::ChartPie));
    actionProcessChecksumsNoFilter->setIcon(m_icons.icon(Icons::Folder));
    actionProcessChecksumsCustomFilter->setIcon(m_icons.icon(Icons::FolderSync));
    actionCheckFileByClipboardChecksum->setIcon(m_icons.icon(Icons::Paste));
    actionProcessSha_toClipboard->setIcon(m_icons.icon(Icons::Copy));
    actionOpenDatabase->setIcon(m_icons.icon(Icons::Database));
    actionCheckSumFile->setIcon(m_icons.icon(Icons::Scan));

    menuAlgo->menuAction()->setIcon(m_icons.icon(Icons::DoubleGear));
    menuCreateDigest->menuAction()->setIcon(m_icons.icon(Icons::Save));

    actionCopyFile->setIcon(m_icons.icon(Icons::Copy));
    actionCopyFolder->setIcon(m_icons.icon(Icons::Copy));

    // DB Model View
    actionCancelBackToFS->setIcon(m_icons.icon(Icons::ProcessAbort));
    actionShowDbStatus->setIcon(m_icons.icon(Icons::Database));
    actionResetDb->setIcon(m_icons.icon(Icons::Undo));
    actionForgetChanges->setIcon(m_icons.icon(Icons::Backup));
    actionCheckCurFileFromModel->setIcon(m_icons.icon(Icons::Scan));
    actionCheckItemSubfolder->setIcon(m_icons.icon(Icons::FolderSync));
    //actionCheckAllMod->setIcon(m_icons.icon(FileStatus::NotCheckedMod));
    actionCheckAll->setIcon(m_icons.icon(Icons::Start));
    actionCopyStoredChecksum->setIcon(m_icons.icon(Icons::Copy));
    actionCopyReChecksum->setIcon(m_icons.icon(Icons::Copy));
    actionBranchMake->setIcon(m_icons.icon(Icons::AddFork));
    actionBranchOpen->setIcon(m_icons.icon(Icons::Branch));
    actionExportSum->setIcon(m_icons.icon(Icons::HashFile));

    actionFilterNewLost->setIcon(m_icons.icon(Icons::NewFile));
    actionFilterMismatches->setIcon(m_icons.icon(Icons::DocClose));
    actionFilterUnreadable->setIcon(m_icons.icon(FileStatus::ReadError));
    actionFilterModified->setIcon(m_icons.icon(FileStatus::NotCheckedMod));

    actionUpdFileAdd->setIcon(m_icons.icon(FileStatus::Added));
    actionUpdFileRemove->setIcon(m_icons.icon(FileStatus::Removed));
    actionUpdFileReChecksum->setIcon(m_icons.icon(FileStatus::Updated));
    actionUpdFileImportDigest->setIcon(m_icons.icon(Icons::HashFile));
    actionUpdFilePasteDigest->setIcon(m_icons.icon(Icons::Paste));

    actionUpdDbAddNew->setIcon(m_icons.icon(FileStatus::Added));
    actionUpdDbClearLost->setIcon(m_icons.icon(FileStatus::Removed));
    actionUpdDbNewLost->setIcon(m_icons.icon(Icons::Update));
    actionUpdDbReChecksums->setIcon(m_icons.icon(FileStatus::Updated));
    actionUpdDbFindMoved->setIcon(m_icons.icon(FileStatus::Moved));
}

void MenuActions::setShortcuts()
{
    // main file menu
    actionChooseFolder->setShortcut(QKeySequence(Qt::ALT | Qt::Key_O));
    actionOpenDatabaseFile->setShortcut(QKeySequence::Open);
    actionSave->setShortcut(QKeySequence::Save);
    actionOpenDialogSettings->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma));
    actionShowFilesystem->setShortcut(Qt::Key_F12);

    // db
    actionShowDbStatus->setShortcut(Qt::Key_F1);
    actionResetDb->setShortcut(Qt::Key_F5);
}

void MenuActions::populateMenuFile(QMenu *menuFile)
{
    menuFile->addActions(m_menuFileActions);
    menuFile->insertSeparator(actionOpenDialogSettings);
    menuFile->insertMenu(actionSave, menuOpenRecent);
}

void MenuActions::updateMenuOpenRecent()
{
    if (m_settings)
        updateMenuOpenRecent(m_settings->recentFiles);
}

void MenuActions::updateMenuOpenRecent(const QStringList &recentFiles)
{
    menuOpenRecent->clear();
    menuOpenRecent->setDisabled(recentFiles.isEmpty());

    if (!menuOpenRecent->isEnabled())
        return;

    QIcon dbIcon = m_icons.icon(Icons::Database);

    for (const QString &recentFilePath : recentFiles) {
        if (QFileInfo::exists(recentFilePath)) {
            QAction *act = menuOpenRecent->addAction(dbIcon, pathstr::basicName(recentFilePath));
            act->setToolTip(recentFilePath);
        }
    }

    menuOpenRecent->addSeparator();
    menuOpenRecent->addAction(actionClearRecent);
}

QMenu* MenuActions::menuUpdateDb(const Numbers &dataNum)
{
    if (!menuUpdateDatabase) {
        menuUpdateDatabase = new QMenu(QStringLiteral(u"Update the Database"));
        menuUpdateDatabase->menuAction()->setIcon(m_icons.icon(Icons::Update));

        menuUpdateDatabase->addAction(actionUpdDbAddNew);
        menuUpdateDatabase->addAction(actionUpdDbClearLost);
        menuUpdateDatabase->addAction(actionUpdDbNewLost);
        menuUpdateDatabase->addSeparator();
        menuUpdateDatabase->addAction(actionUpdDbReChecksums);
    }

    actionUpdDbAddNew->setEnabled(dataNum.contains(FileStatus::New));
    actionUpdDbClearLost->setEnabled(dataNum.contains(FileStatus::Missing));
    actionUpdDbNewLost->setEnabled(actionUpdDbAddNew->isEnabled() && actionUpdDbClearLost->isEnabled());
    actionUpdDbReChecksums->setEnabled(dataNum.contains(FileStatus::Mismatched));

    // show number of items
    static const QString defstrAddNew = actionUpdDbAddNew->text();
    static const QString defstrClearLost = actionUpdDbClearLost->text();
    static const QString defstrUpdateRe = actionUpdDbReChecksums->text();
    static const QString defstrNumFormat = QStringLiteral(u" [%1]");

    QString strAddNew = defstrAddNew + (actionUpdDbAddNew->isEnabled() ? QString(defstrNumFormat)
                                        .arg(dataNum.numberOf(FileStatus::New)) : QString());
    actionUpdDbAddNew->setText(strAddNew);

    QString strClearLost = defstrClearLost + (actionUpdDbClearLost->isEnabled() ? QString(defstrNumFormat)
                                              .arg(dataNum.numberOf(FileStatus::Missing)) : QString());
    actionUpdDbClearLost->setText(strClearLost);

    QString strUpdateRe = defstrUpdateRe + (actionUpdDbReChecksums->isEnabled() ? QString(defstrNumFormat)
                                            .arg(dataNum.numberOf(FileStatus::Mismatched)) : QString());
    actionUpdDbReChecksums->setText(strUpdateRe);

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

    menuAlgo->menuAction()->setText(QStringLiteral(u"Algorithm ") + format::algoToStr(curAlgo));

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
