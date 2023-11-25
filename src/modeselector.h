// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef MODESELECTOR_H
#define MODESELECTOR_H

#include <QObject>
#include "view.h"

class ModeSelector : public QObject
{
    Q_OBJECT
public:
    explicit ModeSelector(View *view, Settings *settings, QObject *parent = nullptr);

    enum Mode {
        NoMode,
        Folder,
        File,
        DbFile,
        SumFile,
        Model,
        ModelNewLost,
        UpdateMismatch,
    };
    Q_ENUM(Mode)

    // modes
    void setMode();
    Mode currentMode();
    bool isProcessing();
    //QSet<Mode> fsModes {Folder, File, DbFile, SumFile};
    //QSet<Mode> dbModes {Model, ModelNewLost, UpdateMismatch};

    // tasks execution
    void quickAction();
    void doWork();

    //---->>>
    void computeFileChecksum(QCryptographicHash::Algorithm algo, bool summaryFile = true, bool clipboard = false);
    void verifyItem();
    void verifyDb();
    void copyStoredChecksumToClipboard();
    void showFolderContentTypes();
    void checkFileChecksum(const QString &checkSum);

    void showFileSystem();

public slots:
    void processing(bool isProcessing);
    void prepareView();

private:
    Mode selectMode(const Numbers &numbers); // select Mode based on the contents of the Numbers struct
    Mode selectMode(const QString &path); // select Mode based on file system path

    Mode curMode;
    bool isProcessing_ = false;
    View *view_;
    Settings *settings_;

signals:
    void setButtonText(const QString &buttonText);

    /////////
    void getPathInfo(const QString &path); // info about folder contents or file (size)
    void getIndexInfo(const QModelIndex &curIndex); // info about database item (file or subfolder index)
    void processFolderSha(const QString &path, QCryptographicHash::Algorithm algo);
    void processFileSha(const QString &path, QCryptographicHash::Algorithm algo, bool summaryFile = true, bool clipboard = false);
    void parseJsonFile(const QString &path);
    void verify(const QModelIndex& index = QModelIndex());
    void updateNewLost();
    void updateMismatch(); // update json Database with new checksums for files with failed verification
    void checkSummaryFile(const QString &path);
    void checkFile(const QString &filePath, const QString &checkSum);
    void copyStoredChecksum(const QModelIndex &fileItemIndex);
    void cancelProcess();
    void resetDatabase(); // reopening and reparsing current database
    void restoreDatabase();
    void dbItemContents(const QString &itemPath);
    void folderContentsByType(const QString &folderPath);
    void dbStatus();
}; // class ModeSelector

using Mode = ModeSelector::Mode;
#endif // MODESELECTOR_H
