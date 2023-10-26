// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include "tools.h"
#include "treemodel.h"
#include "datacontainer.h"

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(Settings *settings, QObject *parent = nullptr);
    ~Manager();

public slots:
    void processFolderSha(const QString &folderPath, QCryptographicHash::Algorithm algo);
    void processFileSha(const QString &filePath, QCryptographicHash::Algorithm algo, bool summaryFile = true, bool clipboard = false);
    void verifyFileList(const QString &subFolder = QString()); // checking the list of files against the checksums stored in the database
    void checkSummaryFile(const QString &path); // path to *.sha1/256/512 summary file
    void checkFile(const QString &filePath, const QString &checkSum);
    void checkFile(const QString &filePath, const QString &checkSum, QCryptographicHash::Algorithm algo);
    void checkCurrentItemSum(const QString &path); // check only selected file instead all database cheking
    QString copyStoredChecksum(const QString &path, bool clipboard = true);
    void getItemInfo(const QString &path); // info about folder contents or file (size)
    void createDataModel(const QString &databaseFilePath); // making tree model | file paths : info about current availability on disk
    void updateNewLost(); // remove lost files, add new files
    void updateMismatch(); // update json Database with new checksums for files with failed verification
    void resetDatabase(); // reopening and reparsing current database
    void restoreDatabase();
    void showNewLostOnly();
    void deleteCurData();
    void isViewFS(const bool isFS);
    void folderContentsByType(const QString &folderPath); // show info message with files number and size by extensions
    void showAll();
    void dbStatus();

private:
    void makeTreeModel(const FileList &data); // populate AbstractItemModel from Map {file path : info}
    void chooseMode(); // if there are New Files or Lost Files --> setMode("modelNewLost"); else setMode("model");
    void showFileCheckResultMessage(bool isMatched);
    QString calculateChecksum(const QString &filePath, QCryptographicHash::Algorithm algo);
    FileList calculateChecksums(DataContainer &filesContainer);
    bool restoreBackup(const QString &databaseFilePath);

    bool canceled = false;
    bool isViewFileSysytem;
    DataMaintainer *curData = nullptr;
    Settings *settings_;

signals:
    void setStatusbarText(const QString &text = QString()); // send the 'text' to statusbar
    void setPermanentStatus(const QString &text = QString());
    void donePercents(int done);
    void procStatus(const QString &str);
    void setModel(TreeModel *model = nullptr);
    void workDirChanged(const QString &workDir);
    void showMessage(const QString &text, const QString &title = "Info");
    void setButtonText(const QString &text);
    void setMode(Mode::Modes mode);
    void toClipboard(const QString &text);
    void cancelProcess();
};

#endif // MANAGER_H
