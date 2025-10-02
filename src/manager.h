/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include "datamaintainer.h"
#include "shacalculator.h"
#include "view.h"
#include "procstate.h"
#include "settings.h"
#include <QElapsedTimer>

struct Task {
    Task(std::function<void()> task, State run_state = State::StartSilently)
        : job(task), state(run_state) {}

    std::function<void()> job;
    State state;
}; // struct Task

class Manager : public QObject
{
    Q_OBJECT

public:
    explicit Manager(Settings *settings, QObject *parent = nullptr);

    // Purpose of file processing (checksum calculation)
    enum DestFileProc { Generic, Clipboard, SumFile };

    enum DbMod {
        DM_AutoSelect = 0,
        DM_AddNew = 1 << 0,
        DM_ClearLost = 1 << 1,
        DM_UpdateNewLost = DM_AddNew | DM_ClearLost,
        DM_UpdateMismatches = 1 << 2,
        DM_FindMoved = 1 << 3,
        DM_ImportDigest = 1 << 4,
        DM_PasteDigest = 1 << 5
    }; // enum DbMod

    DataMaintainer *m_dataMaintainer = new DataMaintainer(this);
    ProcState *m_proc = new ProcState(this);

    template<typename Callable, typename... Args>
    void addTask(Callable&& _func, Args&&... _args)
    {
        Task _task(std::bind(std::forward<Callable>(_func),
                             this, std::forward<Args>(_args)...));
        queueTask(_task);
    }

    template<typename Callable, typename... Args>
    void addTaskWithState(State _state, Callable&& _func, Args&&... _args)
    {
        Task _task(std::bind(std::forward<Callable>(_func),
                             this, std::forward<Args>(_args)...),
                   _state);

        queueTask(_task);
    }

    void clearTasks();

public slots:
    void processFolderSha(const MetaData &metaData);
    void branchSubfolder(const QModelIndex &subfolder);
    void updateDatabase(const Manager::DbMod dest);
    void updateItemFile(const QModelIndex &fileIndex, Manager::DbMod job);
    void importBranch(const QModelIndex &rootFolder);

    void processFileSha(const QString &filePath,
                        QCryptographicHash::Algorithm algo,
                        Manager::DestFileProc result);

    // path to *.sha1/256/512 summary file
    void checkSummaryFile(const QString &path);

    void checkFile(const QString &filePath,
                   const QString &checkSum);

    void checkFile(const QString &filePath,
                   const QString &checkSum,
                   QCryptographicHash::Algorithm algo);

    void createDataModel(const QString &dbFilePath);
    void restoreDatabase();
    void saveData();
    void prepareSwitchToFs();
    void makeDbContentsList();

    // info about file (size) or folder contents
    void getPathInfo(const QString &path);

    // info about database item (the file or subfolder index)
    void getIndexInfo(const QModelIndex &curIndex);

    // recive the signal when Model has been changed
    void modelChanged(View::ModelView modelView);

    // checking the list of files against the checksums stored in the database
    void verifyFolderItem(const QModelIndex &folderItemIndex, Files::FileStatus checkstatus);

    // check only selected file instead of full database verification
    void verifyFileItem(const QModelIndex &fileItemIndex);

    // make a list of the file types contained in the folder, their number and size
    void folderContentsList(const QString &folderPath, bool filterCreation);

    void cacheMissingItems();

    void runTasks();

private:
    enum CalcKind : quint8 { Calculation, Verification };

    void queueTask(Task task);
    void sendDbUpdated();

    void showFileCheckResultMessage(const QString &filePath,
                                    const QString &checksumEstimated,
                                    const QString &checksumCalculated);

    void calcFailedMessage(const QString &filePath);

    QString hashFile(const QString &filePath,
                     QCryptographicHash::Algorithm algo,
                     const CalcKind calckind = Calculation);

    QString hashItem(const QModelIndex &ind,
                     const CalcKind calckind = Calculation);

    int calculateChecksums(const FileStatus status,
                           const QModelIndex &root = QModelIndex());

    int calculateChecksums(const DbMod purpose,
                           const FileStatus status,
                           const QModelIndex &root = QModelIndex());

    void updateProgText(const CalcKind calckind, const QString &file);
    QString extractDigestFromFile(const QString &digest_file);

    // variables
    bool m_isViewFileSysytem;
    Settings *m_settings = nullptr;
    Files *m_files = new Files(this);
    ShaCalculator m_shaCalc;
    QList<Task> m_taskQueue;
    QElapsedTimer m_elapsedTimer;

    const QString k_movedDbWarning = QStringLiteral(
        u"The database file may have been moved or refers to an inaccessible location.");

signals:
    // sends the 'text' to statusbar
    void setStatusbarText(const QString &text = QString());

    // sends the data (models) to the tree View
    void setViewData(DataContainer *data = nullptr);

    void folderContentsListCreated(const QString &folderPath, const FileTypeList &extList);
    void dbCreationDataCollected(const QString &folderPath, const QStringList &dbFiles, const FileTypeList &extList);
    void dbContentsListCreated(const QString &folderPath, const FileTypeList &extList);
    void folderChecked(const Numbers &result, const QString &subFolder = QString());
    void fileProcessed(const QString &fileName, const FileValues &result);
    void showMessage(const QString &text, const QString &title = QStringLiteral(u"Info"));
    void switchToFsPrepared();
    void mismatchFound();
    void taskAdded();
}; // class Manager

using DestFileProc = Manager::DestFileProc;
using DbMod = Manager::DbMod;

#endif // MANAGER_H
