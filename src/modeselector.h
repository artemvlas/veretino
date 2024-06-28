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

    void setProcState(ProcState *procState);
    void abortProcess();
    void stopProcess();

    Mode mode() const;
    bool isMode(const Modes expected);

    QString getButtonText();
    QString getButtonToolTip();
    QIcon getButtonIcon();

    // tasks execution
    void quickAction();
    void doWork();
    void processChecksumsNoFilter();
    void processChecksumsPermFilter();
    void processChecksumsFiltered();
    void processFolderChecksums(const FilterRule &filter);
    void openFsPath(const QString &path);
    void openJsonDatabase(const QString &filePath);
    void openRecentDatabase(const QAction *action);
    void openBranchDb();

    // prompts
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
    void copyItem();

    void showFileSystem();

    IconProvider iconProvider;
    MenuActions *menuAct_ = new MenuActions(this);

public slots:
    void createContextMenu_View(const QPoint &point);
    void getInfoPathItem();

private:
    void connectActions();
    void copyDataToClipboard(Column column);
    void updateDbItem();
    bool promptMessageProcCancelation_(bool abort);

    QString composeDbFilePath();
    bool isSelectedCreateDb();

    void createContextMenu_ViewFs(const QPoint &point);
    void createContextMenu_ViewDb(const QPoint &point);

    View *view_;
    Settings *settings_;
    ProcState *proc_ = nullptr;

signals:
    void getPathInfo(const QString &path); // info about folder contents or file (size)
    void getIndexInfo(const QModelIndex &curIndex); // info about database item (file or subfolder index)
    void processFolderSha(const MetaData &metaData);
    void processFileSha(const QString &path, QCryptographicHash::Algorithm algo, DestFileProc result = DestFileProc::Generic);
    void parseJsonFile(const QString &path);
    void verify(const QModelIndex& index = QModelIndex());
    void updateDatabase(const DestDbUpdate task);
    void updateItemFile(const QModelIndex &fileIndex);
    void checkSummaryFile(const QString &path);
    void checkFile(const QString &filePath, const QString &checkSum);
    void resetDatabase(); // reopening and reparsing current database
    void restoreDatabase();
    void dbItemContents(const QString &itemPath);
    void makeFolderContentsList(const QString &folderPath);
    void makeFolderContentsFilter(const QString &folderPath);
    void makeDbContentsList();
    void branchSubfolder(const QModelIndex &subfolder);
    void makeSumFile(const QString &originFilePath, const QString &checksum);
    void saveData();
    void prepareSwitchToFs();
}; // class ModeSelector

using Mode = ModeSelector::Mode;
Q_DECLARE_OPERATORS_FOR_FLAGS(ModeSelector::Modes)

#endif // MODESELECTOR_H
