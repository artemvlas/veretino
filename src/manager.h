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

    DataMaintainer *dataMaintainer = new DataMaintainer(this);

public slots:
    void processFolderSha(const QString &folderPath, QCryptographicHash::Algorithm algo);
    void processFileSha(const QString &filePath, QCryptographicHash::Algorithm algo, bool summaryFile = true, bool clipboard = false);
    void verify(const QModelIndex &curIndex);
    void checkSummaryFile(const QString &path); // path to *.sha1/256/512 summary file
    void checkFile(const QString &filePath, const QString &checkSum);
    void checkFile(const QString &filePath, const QString &checkSum, QCryptographicHash::Algorithm algo);

    void getPathInfo(const QString &path); // info about folder contents or file (size)
    void getIndexInfo(const QModelIndex &curIndex); // info about database item (file or subfolder index)
    void createDataModel(const QString &databaseFilePath); // making tree model | file paths : info about current availability on disk
    void updateNewLost(); // remove lost files, add new files
    void updateMismatch(); // update json Database with new checksums for files with failed verification
    void resetDatabase(); // reopening and reparsing current database
    void restoreDatabase();
    void modelChanged(ModelView modelView); // recive the signal when Model has been changed
    void folderContentsByType(const QString &folderPath); // show info message with files number and size by extensions

private:
    void verifyFolderItem(const QModelIndex &folderItemIndex = QModelIndex()); // checking the list of files against the checksums stored in the database
    void verifyFileItem(const QModelIndex &fileItemIndex); // check only selected file instead all database cheking
    void showFileCheckResultMessage(bool isMatched, const QString &fileName = QString());

    QString calculateChecksum(const QString &filePath, QCryptographicHash::Algorithm algo,
                              bool finalProcess = true); // <finalProcess> -->> whether it sends a process end signal or not
    int calculateChecksums(FileStatus status = FileStatus::Queued,
                           bool finalProcess = true);
    int calculateChecksums(QModelIndex rootIndex,
                           FileStatus status = FileStatus::Queued,
                           bool finalProcess = true);

    bool canceled = false;
    bool isViewFileSysytem;
    Settings *settings_;

signals:
    void processing(bool isProcessing, bool visibleProgress = false);
    void cancelProcess();
    void setStatusbarText(const QString &text = QString()); // send the 'text' to statusbar
    void setPermanentStatus(const QString &text = QString());
    void donePercents(int done);
    void procStatus(const QString &str);
    void setViewData(DataContainer *data = nullptr, bool isImported = true); // isImported == true, if the data is obtained from a database file
    void setTreeModel(ModelView modelSel = ModelView::ModelProxy);
    void folderContentsListCreated(const QString &folderPath, const QList<ExtNumSize> &extList);
    void showMessage(const QString &text, const QString &title = "Info");
    void toClipboard(const QString &text); // Sending directly to QGuiApplication::clipboard()->setText works great on Linux,
                                           // but does NOT work on older QT builds on Windows. So this signal is used for compatibility.
};

#endif // MANAGER_H
