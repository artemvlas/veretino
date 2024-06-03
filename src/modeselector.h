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
        Processing = 1 << 1,
        Folder = 1 << 2,
        File = 1 << 3,
        DbFile = 1 << 4,
        SumFile = 1 << 5,
        Model = 1 << 6,
        ModelNewLost = 1 << 7,
        UpdateMismatch = 1 << 8,
        // FlagDbModes = Model | ModelNewLost | UpdateMismatch
    };
    Q_ENUM(Mode)
    Q_DECLARE_FLAGS(Modes, Mode)

    void setProcState(ProcState *procState);
    void cancelProcess();
    void abortProcess();

    Mode mode();
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
    bool processAbortPrompt();
    bool overwriteDbPrompt();
    bool emptyFolderPrompt();

    void procSumFile(QCryptographicHash::Algorithm algo);
    void verifyItem();
    void verifyDb();
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

    QString composeDbFilePath();
    bool isSelectedCreateDb();

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
    void checkSummaryFile(const QString &path);
    void checkFile(const QString &filePath, const QString &checkSum);
    void resetDatabase(); // reopening and reparsing current database
    void restoreDatabase();
    void dbItemContents(const QString &itemPath);
    void makeFolderContentsList(const QString &folderPath);
    void makeFolderContentsFilter(const QString &folderPath);
    void branchSubfolder(const QModelIndex &subfolder);
    void makeSumFile(const QString &originFilePath, const QString &checksum);
    void saveData();
}; // class ModeSelector

using Mode = ModeSelector::Mode;
Q_DECLARE_OPERATORS_FOR_FLAGS(ModeSelector::Modes)

#endif // MODESELECTOR_H
