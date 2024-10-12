/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include "datamaintainer.h"
#include "view.h"
#include "procstate.h"
#include "settings.h"

struct Task {
    Task(std::function<void()> func, State state = State::StartSilently)
        : _func(func), _state(state) {}

    std::function<void()> _func;
    State _state;
}; // struct Task

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(Settings *settings, QObject *parent = nullptr);

    enum DestFileProc { Generic, Clipboard, SumFile }; // Purpose of file processing (checksum calculation)

    enum DestDbUpdate {
        DestUpdateMismatches = 1 << 0,
        DestAddNew = 1 << 1,
        DestClearLost = 1 << 2,
        DestUpdateNewLost = DestAddNew | DestClearLost
    };

    DataMaintainer *dataMaintainer = new DataMaintainer(this);
    ProcState *procState = new ProcState(this);

    template<typename Callable, typename... Args>
    void addTask(Callable&& _func, Args&&... _args)
    {
        Task _task(std::bind(std::forward<Callable>(_func),
                             this, std::forward<Args>(_args)...));
        queueTask(_task);
    }

    template<typename Callable, typename... Args>
    void addTaskWithState(State p_state, Callable&& _func, Args&&... _args)
    {
        Task _task(std::bind(std::forward<Callable>(_func),
                             this, std::forward<Args>(_args)...),
                   p_state);

        queueTask(_task);
    }

    void clearTasks();

public slots:
    void processFolderSha(const MetaData &metaData);
    void branchSubfolder(const QModelIndex &subfolder);
    void updateDatabase(const DestDbUpdate dest);
    void updateItemFile(const QModelIndex &fileIndex);

    void processFileSha(const QString &filePath, QCryptographicHash::Algorithm algo, DestFileProc result);
    void checkSummaryFile(const QString &path); // path to *.sha1/256/512 summary file
    void checkFile(const QString &filePath, const QString &checkSum);
    void checkFile(const QString &filePath, const QString &checkSum, QCryptographicHash::Algorithm algo);

    void createDataModel(const QString &dbFilePath);
    void restoreDatabase();
    void saveData();
    void prepareSwitchToFs();

    void getPathInfo(const QString &path); // info about file (size) or folder contents
    void getIndexInfo(const QModelIndex &curIndex); // info about database item (the file or subfolder index)
    void modelChanged(ModelView modelView); // recive the signal when Model has been changed
    void makeDbContentsList();

    // checking the list of files against the checksums stored in the database
    void verifyFolderItem(const QModelIndex &folderItemIndex, FileStatus checkstatus);

    void verifyFileItem(const QModelIndex &fileItemIndex); // check only selected file instead of full database verification

    void folderContentsList(const QString &folderPath, bool filterCreation); // make a list of the file types contained in the folder, their number and size

    void runTasks();

private:
    void queueTask(Task task);
    void sendDbUpdated();

    void showFileCheckResultMessage(const QString &filePath,
                                    const QString &checksumEstimated,
                                    const QString &checksumCalculated);

    QString calculateChecksum(const QString &filePath,
                              QCryptographicHash::Algorithm algo,
                              bool isVerification = false);

    int calculateChecksums(FileStatus status, const QModelIndex &rootIndex = QModelIndex());
    void updateCalcStatus(const QString &_purp, Pieces<int> _p_items);

    // variables
    bool isViewFileSysytem;
    Settings *settings_;
    Files *files_ = new Files(this);
    QList<Task> taskQueue_;

    const QString movedDbWarning = "The database file may have been moved or refers to an inaccessible location.";

signals:
    void setStatusbarText(const QString &text = QString()); // send the 'text' to statusbar
    void setViewData(DataContainer *data = nullptr);
    void folderContentsListCreated(const QString &folderPath, const FileTypeList &extList);
    void dbCreationDataCollected(const QString &folderPath, const QStringList &dbFiles, const FileTypeList &extList);
    void dbContentsListCreated(const QString &folderPath, const FileTypeList &extList);
    void folderChecked(const Numbers &result, const QString &subFolder = QString());
    void fileProcessed(const QString &fileName, const FileValues &result);
    void showMessage(const QString &text, const QString &title = "Info");
    void finishedCalcFileChecksum();
    void switchToFsPrepared();
    void mismatchFound();
    void taskAdded();
}; // class Manager

using DestFileProc = Manager::DestFileProc;
using DestDbUpdate = Manager::DestDbUpdate;

#endif // MANAGER_H
