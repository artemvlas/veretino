#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include "shacalculator.h"
#include "jsondb.h"
#include "treemodel.h"
#include "QThread"

namespace tools {
int shaStrLen(int shatype); // returns the length of the checksum string depending on the sha-type: sha(1) = 40, sha(256) = 64, sha(512) = 128
} // namespace tools

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
    void processFileSha(const QString &filePath, int shatype);
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
    void aboutDatabase();

signals:
    void status(const QString &text = QString()); // text to statusbar
    void donePercents(int done);
    void setModel(TreeModel *model = nullptr);
    void workDirChanged(const QString &workDir);
    void showMessage(const QString &text, const QString &title = "Info");
    void setButtonText(const QString &text);
    void setMode(const QString &mode); //jsonModel,
    void toClipboard(const QString &text);
    void cancelProcess();
};

#endif // MANAGER_H
