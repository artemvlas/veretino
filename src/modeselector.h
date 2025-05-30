/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef MODESELECTOR_H
#define MODESELECTOR_H

#include <QObject>
#include "view.h"
#include "manager.h"
#include "menuactions.h"

class ModeSelector : public QObject
{
    Q_OBJECT

public:
    explicit ModeSelector(View *view, Settings *settings, QObject *parent = nullptr);

    enum Mode {
        NoMode = 1 << 0,
        FileProcessing = 1 << 1,
        DbCreating = 1 << 2,
        DbProcessing = 1 << 3,
        Folder = 1 << 4,
        File = 1 << 5,
        DbFile = 1 << 6,
        SumFile = 1 << 7,
        Model = 1 << 8,
        ModelNewLost = 1 << 9,
        UpdateMismatch = 1 << 10,
        DbIdle = Model | ModelNewLost | UpdateMismatch
    };
    Q_ENUM(Mode)
    Q_DECLARE_FLAGS(Modes, Mode)

    void setManager(Manager *manager);
    void setProcState(ProcState *procState);
    void abortProcess();
    void stopProcess();

    Mode mode() const;
    bool isMode(const Modes expected);
    bool isDbConst() const;

    QString getButtonText();
    QString getButtonToolTip();
    QIcon getButtonIcon();

    /*** tasks execution ***/
    void quickAction();
    void doWork();
    void processChecksumsNoFilter();
    void processChecksumsFiltered();
    void processFolderChecksums(const FilterRule &filter);
    void openJsonDatabase(const QString &filePath);
    void openRecentDatabase(const QAction *action);
    void openBranchDb();
    void importBranch();
    void makeDbContList();

    // reopening and reparsing current database
    void resetDatabase();
    void restoreDatabase();
    void updateDatabase(const DbMod task);
    void processFileSha(const QString &path,
                        QCryptographicHash::Algorithm algo,
                        DestFileProc result = DestFileProc::Generic);
    void checkSummaryFile(const QString &path);
    void checkFile(const QString &filePath, const QString &checkSum);
    void verify(const QModelIndex index = QModelIndex());
    void verifyModified();
    void verifyItems(const QModelIndex &root, FileStatus status);
    void branchSubfolder();

    void makeFolderContentsList(const QString &folderPath);
    void makeFolderContentsFilter(const QString &folderPath);

    void saveData();

    /*** prompts ***/
    bool promptProcessStop();
    bool promptProcessAbort();
    bool overwriteDbPrompt();
    bool emptyFolderPrompt();

    void procSumFile(QCryptographicHash::Algorithm algo);
    void verifyItem();
    void verifyDb();
    void promptItemFileUpd();
    void showFolderContentTypes();
    void checkFileByClipboardChecksum();
    void copyFsItem();

    IconProvider m_icons;
    MenuActions *m_menuAct = new MenuActions(this);

public slots:
    void showFileSystem(const QString &path = QString());
    void createContextMenu_View(const QPoint &point);
    void getInfoPathItem();

private:
    void connectActions();
    void copyDataToClipboard(Column column);
    void updateDbItem();
    void updateItemFile(DbMod job);
    void exportItemSum();
    void importItemSum();
    void pasteItemSum();
    bool promptMessageProcCancelation_(bool abort);

    QString composeDbFilePath();
    bool isSelectedCreateDb();

    void createContextMenu_ViewFs(const QPoint &point);
    void createContextMenu_ViewDb(const QPoint &point);

    // returns the digest string if there is one on the clipboard
    QString copiedDigest() const;

    // additionally checks whether the length matches the algorithm
    QString copiedDigest(QCryptographicHash::Algorithm algo) const;

    View *p_view;
    Settings *p_settings;
    ProcState *p_proc = nullptr;
    Manager *p_manager = nullptr;

}; // class ModeSelector

using Mode = ModeSelector::Mode;
Q_DECLARE_OPERATORS_FOR_FLAGS(ModeSelector::Modes)

#endif // MODESELECTOR_H
