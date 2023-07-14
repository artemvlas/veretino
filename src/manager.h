// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include "shacalculator.h"
#include "jsondb.h"
#include "treemodel.h"
#include "QThread"

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject *parent = nullptr);
    ~Manager();

private:
    ShaCalculator *shaCalc = new ShaCalculator;
    void connections();
    DataMaintainer *curData = nullptr;
    void makeTreeModel(const FileList &data); // populate AbstractItemModel from Map {file path : info}
    QVariantMap settings;
    void setMode_model(); // if there are New Files or Lost Files --> setMode("modelNewLost"); else setMode("model");
    bool isViewFileSysytem;

public slots:
    void processFolderSha(const QString &folderPath, int shatype);
    void processFileSha(const QString &filePath, int shatype = 0, bool summaryFile = true, bool clipboard = false);
    void checkFileSummary(const QString &path); // path to *.sha1/256/512 summary file
    void checkCurrentItemSum(const QString &path); // check only selected file instead all database cheking
    QString copyStoredChecksum(const QString &path, bool clipboard = true);
    void getItemInfo(const QString &path); // info about folder contents or file (size)
    void createDataModel(const QString &databaseFilePath); // making tree model | file paths : info about current availability on disk
    void verifyFileList(); // checking the list of files against the checksums stored in the database
    void updateNewLost(); // remove lost files, add new files
    void updateMismatch(); // update json Database with new checksums for files with failed verification
    void getSettings(const QVariantMap &settingsMap);
    void resetDatabase(); // reopening and reparsing current database
    void showNewLostOnly();
    void deleteCurData();
    void isViewFS(const bool isFS);
    void folderContentsByType(const QString &folderPath); // show info message with files number and size by extensions
    void showAll();
    void dbStatus();

signals:
    void setStatusbarText(const QString &text = QString()); // text to statusbar
    void setPermanentStatus(const QString &text = QString());
    void donePercents(int done);
    void setModel(TreeModel *model = nullptr);
    void workDirChanged(const QString &workDir);
    void showMessage(const QString &text, const QString &title = "Info");
    void setButtonText(const QString &text);
    void setMode(int mode);
    void toClipboard(const QString &text);
    void setItemStatus(const QString &itemPath, int status);
    void cancelProcess();
};

#endif // MANAGER_H
