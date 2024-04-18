/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include "tools.h"
#include "treemodel.h"
#include "datamaintainer.h"
#include "view.h"

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(Settings *settings, QObject *parent = nullptr);
    enum ProcFileResult {Generic, Clipboard, SumFile};
    enum TaskDbUpdate {TaskUpdateMismatches, TaskUpdateNewLost, TaskAddNew, TaskClearLost};
    DataMaintainer *dataMaintainer = new DataMaintainer(this);

public slots:
    void processFolderSha(const MetaData &metaData);
    void processFileSha(const QString &filePath, QCryptographicHash::Algorithm algo, ProcFileResult result);
    void verify(const QModelIndex &curIndex);
    void checkSummaryFile(const QString &path); // path to *.sha1/256/512 summary file
    void checkFile(const QString &filePath, const QString &checkSum);
    void checkFile(const QString &filePath, const QString &checkSum, QCryptographicHash::Algorithm algo);

    void getPathInfo(const QString &path); // info about file (size) or folder contents
    void getIndexInfo(const QModelIndex &curIndex); // info about database item (the file or subfolder index)
    void createDataModel(const QString &databaseFilePath); // making the tree data model
    void updateDatabase(const TaskDbUpdate task);
    void resetDatabase(); // reopening and reparsing current database
    void restoreDatabase();
    void modelChanged(ModelView modelView); // recive the signal when Model has been changed
    void makeFolderContentsList(const QString &folderPath);
    void makeFolderContentsFilter(const QString &folderPath);

private:
    void verifyFolderItem(const QModelIndex &folderItemIndex = QModelIndex()); // checking the list of files against the checksums stored in the database
    void verifyFileItem(const QModelIndex &fileItemIndex); // check only selected file instead of full database verification
    void showFileCheckResultMessage(const QString &filePath, const QString &checksumEstimated, const QString &checksumCalculated);
    void folderContentsList(const QString &folderPath, bool filterCreation); // make a list of the file types contained in the folder, their number and size

    QString calculateChecksum(const QString &filePath, QCryptographicHash::Algorithm algo,
                              bool finalProcess = true, bool isVerification = false); // <finalProcess> -->> whether it sends a process end signal or not
    int calculateChecksums(FileStatus status = FileStatus::Queued,
                           bool finalProcess = true);
    int calculateChecksums(const QModelIndex &rootIndex,
                           FileStatus status = FileStatus::Queued,
                           bool finalProcess = true);

    bool canceled = false;
    bool isViewFileSysytem;
    Settings *settings_;

    const QString movedDbWarning = "The database file may have been moved or refers to an inaccessible location.";

signals:
    void processing(bool isProcessing, bool visibleProgress = false);
    void cancelProcess();
    void setStatusbarText(const QString &text = QString()); // send the 'text' to statusbar
    void donePercents(int done);
    void procStatus(const QString &str);
    void setViewData(DataContainer *data = nullptr);
    void setTreeModel(ModelView modelSel = ModelView::ModelProxy);
    void folderContentsListCreated(const QString &folderPath, const QList<ExtNumSize> &extList);
    void folderContentsFilterCreated(const QString &folderPath, const QList<ExtNumSize> &extList);
    void folderChecked(const Numbers &result, const QString &subFolder = QString());
    void fileProcessed(const QString &fileName, const FileValues &result);
    void showMessage(const QString &text, const QString &title = "Info");
}; // class Manager

using ProcFileResult = Manager::ProcFileResult;
using TaskDbUpdate = Manager::TaskDbUpdate;

#endif // MANAGER_H
