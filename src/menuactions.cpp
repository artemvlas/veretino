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
    actionFilterUnreadable->setCheckable(true);
    actionFilterModified->setCheckable(true);

    actionSetAlgoSha1->setCheckable(true);
    actionSetAlgoSha256->setCheckable(true);
    actionSetAlgoSha512->setCheckable(true);

    actionGroupSelectAlgo->addAction(actionSetAlgoSha1);
    actionGroupSelectAlgo->addAction(actionSetAlgoSha256);
    actionGroupSelectAlgo->addAction(actionSetAlgoSha512);

    menuAlgo->addActions(actionGroupSelectAlgo->actions());
    menuCreateDigest->addActions(actionsMakeDigest);

    menuOpenRecent->setToolTipsVisible(true);

    setShortcuts();

    actionSave->setEnabled(false);
}

MenuActions::~MenuActions()
{
    qDeleteAll(listOfMenus);
}

void MenuActions::setIconTheme(const QPalette &palette)
{
    _icons.setTheme(palette);
    setActionsIcons();
}

void MenuActions::setSettings(const Settings *settings)
{
    settings_ = settings;
}

void MenuActions::setActionsIcons()
{
    // MainWindow menu
    actionChooseFolder->setIcon(_icons.icon(Icons::Folder));
    actionOpenDatabaseFile->setIcon(_icons.icon(Icons::Database));
    actionSave->setIcon(_icons.icon(Icons::Save));
    actionShowFilesystem->setIcon(_icons.icon(Icons::FileSystem));
    actionOpenDialogSettings->setIcon(_icons.icon(Icons::Configure));

    menuOpenRecent->menuAction()->setIcon(_icons.icon(Icons::Clock));
    actionClearRecent->setIcon(_icons.icon(Icons::ClearHistory));
    actionAbout->setIcon(_icons.icon(Icons::Info));

    // File system View
    actionToHome->setIcon(_icons.icon(Icons::GoHome));
    actionStop->setIcon(_icons.icon(Icons::ProcessStop));
    actionShowFolderContentsTypes->setIcon(_icons.icon(Icons::ChartPie));
    actionProcessChecksumsNoFilter->setIcon(_icons.icon(Icons::Folder));
    actionProcessChecksumsCustomFilter->setIcon(_icons.icon(Icons::FolderSync));
    actionCheckFileByClipboardChecksum->setIcon(_icons.icon(Icons::Paste));
    actionProcessSha_toClipboard->setIcon(_icons.icon(Icons::Copy));
    actionOpenDatabase->setIcon(_icons.icon(Icons::Database));
    actionCheckSumFile->setIcon(_icons.icon(Icons::Scan));

    menuAlgo->menuAction()->setIcon(_icons.icon(Icons::DoubleGear));
    menuCreateDigest->menuAction()->setIcon(_icons.icon(Icons::Save));

    actionCopyFile->setIcon(_icons.icon(Icons::Copy));
    actionCopyFolder->setIcon(_icons.icon(Icons::Copy));

    // DB Model View
    actionCancelBackToFS->setIcon(_icons.icon(Icons::ProcessAbort));
    actionShowDbStatus->setIcon(_icons.icon(Icons::Database));
    actionResetDb->setIcon(_icons.icon(Icons::Undo));
    actionForgetChanges->setIcon(_icons.icon(Icons::Backup));
    actionCheckCurFileFromModel->setIcon(_icons.icon(Icons::Scan));
    actionCheckCurSubfolderFromModel->setIcon(_icons.icon(Icons::FolderSync));
    //actionCheckAllMod->setIcon(_icons.icon(FileStatus::NotCheckedMod));
    actionCheckAll->setIcon(_icons.icon(Icons::Start));
    actionCopyStoredChecksum->setIcon(_icons.icon(Icons::Copy));
    actionCopyReChecksum->setIcon(_icons.icon(Icons::Copy));
    actionBranchMake->setIcon(_icons.icon(Icons::AddFork));
    actionBranchOpen->setIcon(_icons.icon(Icons::Branch));
    actionExportSum->setIcon(_icons.icon(Icons::HashFile));

    actionFilterNewLost->setIcon(_icons.icon(Icons::NewFile));
    actionFilterMismatches->setIcon(_icons.icon(Icons::DocClose));
    actionFilterUnreadable->setIcon(_icons.icon(FileStatus::ReadError));
    actionFilterModified->setIcon(_icons.icon(FileStatus::NotCheckedMod));

    actionUpdFileAdd->setIcon(_icons.icon(FileStatus::Added));
    actionUpdFileRemove->setIcon(_icons.icon(FileStatus::Removed));
    actionUpdFileReChecksum->setIcon(_icons.icon(FileStatus::Updated));
    actionUpdFileImportDigest->setIcon(_icons.icon(Icons::HashFile));

    actionUpdDbAddNew->setIcon(_icons.icon(FileStatus::Added));
    actionUpdDbClearLost->setIcon(_icons.icon(FileStatus::Removed));
    actionUpdDbNewLost->setIcon(_icons.icon(Icons::Update));
    actionUpdDbReChecksums->setIcon(_icons.icon(FileStatus::Updated));
    actionUpdDbFindMoved->setIcon(_icons.icon(FileStatus::Moved));
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
    menuFile->addActions(menuFileActions);
    menuFile->insertSeparator(actionOpenDialogSettings);
    menuFile->insertMenu(actionSave, menuOpenRecent);
}

void MenuActions::updateMenuOpenRecent()
{
    if (settings_)
        updateMenuOpenRecent(settings_->recentFiles);
}

void MenuActions::updateMenuOpenRecent(const QStringList &recentFiles)
{
    menuOpenRecent->clear();
    menuOpenRecent->setDisabled(recentFiles.isEmpty());

    if (!menuOpenRecent->isEnabled())
        return;

    QIcon dbIcon = _icons.icon(Icons::Database);

    for (const QString &recentFilePath : recentFiles) {
        if (QFileInfo::exists(recentFilePath)) {
            QAction *act = menuOpenRecent->addAction(dbIcon, paths::basicName(recentFilePath));
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
        menuUpdateDatabase->menuAction()->setIcon(_icons.icon(Icons::Update));

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

    QString _strAddNew = defstrAddNew + (actionUpdDbAddNew->isEnabled() ? QString(defstrNumFormat)
                                         .arg(dataNum.numberOf(FileStatus::New)) : QString());
    actionUpdDbAddNew->setText(_strAddNew);

    QString _strClearLost = defstrClearLost + (actionUpdDbClearLost->isEnabled() ? QString(defstrNumFormat)
                                                .arg(dataNum.numberOf(FileStatus::Missing)) : QString());
    actionUpdDbClearLost->setText(_strClearLost);

    QString _strUpdateRe = defstrUpdateRe + (actionUpdDbReChecksums->isEnabled() ? QString(defstrNumFormat)
                                                        .arg(dataNum.numberOf(FileStatus::Mismatched)) : QString());
    actionUpdDbReChecksums->setText(_strUpdateRe);

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
