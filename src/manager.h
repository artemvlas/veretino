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
        std::function<void()> _task = std::bind(std::forward<Callable>(_func), this, std::forward<Args>(_args)...);
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

    void createDataModel(const QString &databaseFilePath);
    void restoreDatabase();
    void saveData();
    void prepareSwitchToFs();

    void getPathInfo(const QString &path); // info about file (size) or folder contents
    void getIndexInfo(const QModelIndex &curIndex); // info about database item (the file or subfolder index)
    void modelChanged(ModelView modelView); // recive the signal when Model has been changed
    void makeDbContentsList();

    void verifyFolderItem(const QModelIndex &folderItemIndex = QModelIndex()); // checking the list of files against the checksums stored in the database
    void verifyFileItem(const QModelIndex &fileItemIndex); // check only selected file instead of full database verification

    void folderContentsList(const QString &folderPath, bool filterCreation); // make a list of the file types contained in the folder, their number and size

    void runTasks();

private:
    void queueTask(std::function<void()> task);

    void showFileCheckResultMessage(const QString &filePath, const QString &checksumEstimated, const QString &checksumCalculated);
    QString calculateChecksum(const QString &filePath, QCryptographicHash::Algorithm algo,
                              bool isVerification = false);

    int calculateChecksums(FileStatus status);
    int calculateChecksums(const QModelIndex &rootIndex, FileStatus status);

    // variables
    bool isViewFileSysytem;
    Settings *settings_;
    Files *files_ = new Files(this);
    QList<std::function<void()>> taskQueue_;

    const QString movedDbWarning = "The database file may have been moved or refers to an inaccessible location.";

signals:
    void setStatusbarText(const QString &text = QString()); // send the 'text' to statusbar
    void setViewData(DataContainer *data = nullptr);
    void setTreeModel(ModelView modelSel = ModelView::ModelProxy);
    void folderContentsListCreated(const QString &folderPath, const QList<ExtNumSize> &extList);
    void folderContentsFilterCreated(const QString &folderPath, const QList<ExtNumSize> &extList);
    void dbContentsListCreated(const QString &folderPath, const QList<ExtNumSize> &extList);
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
