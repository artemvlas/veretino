/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef FILES_H
#define FILES_H

#include <QObject>
#include <QMap>
#include <QAbstractItemModel>
#include "filterrule.h"

struct FileValues;
struct ExtNumSize;
using FileList = QMap<QString, FileValues>; // {relative path to file : FileValues struct}

class Files : public QObject
{
    Q_OBJECT
public:
    explicit Files(QObject *parent = nullptr);
    explicit Files(const QString &initPath, QObject *parent = nullptr);
    Files(const FileList &fileList, QObject *parent = nullptr);

    enum FileStatus {
        NotSet = 0,

        Queued = 1 << 0, // added to the processing queue
        Calculating = 1 << 1, // checksum is being calculated
        Verifying = 1 << 2, // // checksum is being verified
        FlagProcessing = Queued | Calculating | Verifying,

        NotChecked = 1 << 3, // available for verification
        Matched = 1 << 4, // checked, checksum matched
        Mismatched = 1 << 5, // checked, checksum did not match
        New = 1 << 6, // a file that is present on disk but not in the database
        Missing = 1 << 7, // not on disk, but present in the database
        Unreadable = 1 << 8, // present on the disk, but unreadable (no read permission or read error)
        Added = 1 << 9, // item (file path and its checksum) has been added to the database
        Removed = 1 << 10, // item^ removed from the database
        Updated = 1 << 11, // the checksum has been updated

        FlagAvailable = NotChecked | Matched | Mismatched | Added | Updated,
        FlagHasChecksum = FlagAvailable | Missing,
        FlagUpdatable = New | Missing | Mismatched,
        FlagDbChanged = Added | Removed | Updated,
        FlagChecked = Matched | Mismatched,
        FlagMatched = Matched | Added | Updated,
        FlagNewLost = New | Missing,

        Computed = 1 << 12, // the checksum has been calculated and is ready for further processing (copy or save)
        ToClipboard = 1 << 13, // the calculated checksum is intended to be copied to the clipboard
        ToSumFile = 1 << 14, // the calculated checksum is intended to be stored in the summary file
    };
    Q_ENUM(FileStatus)
    Q_DECLARE_FLAGS(FileStatuses, FileStatus)

    // functions
    FileList getFileList(); // 'initFolderPath' --> getFileList(const QString &rootFolder)
    FileList getFileList(const QString &rootFolder); // iterate the 'rootFolder', returns all files list
    FileList getFileList(const FilterRule &filter); // return filtered filelist: can ignore or include only files with specified extensions
    FileList getFileList(const QString &rootFolder, const FilterRule &filter);
    FileList getFileList(const FileList &fileList, const FilterRule &filter);

    static bool isEmptyFolder(const QString &folderPath, const FilterRule &filter = FilterRule(false)); // checks whether there are any (or filtered) files the folder/subfolders

    qint64 dataSize(); // total size of all files in the 'initFolderPath' or 'initFileList'
    qint64 dataSize(const QString &folder); // total size of getFileList('folder')
    static qint64 dataSize(const FileList &filelist); // total size of all files in the 'filelist'

    QString contentStatus(const QString &path);
    static QString contentStatus(const FileList &filelist);
    static QString itemInfo(const QAbstractItemModel *model, const FileStatuses flags = FileStatus::NotSet,
                            const QModelIndex &rootIndex = QModelIndex());

    // variables
    bool canceled = false;

public slots:
    void cancelProcess();
    QString contentStatus(); // returns "filename (readable size)" for file, or "folder name: number of files (redable size) for folders"
    void folderContentsByType(); // returns sorted by data size list of extensions, files number and their size

private:
    // variables
    QString initFilePath; // path to the File specified when creating the object
    QString initFolderPath; // path to the Folder specified when creating the object
    FileList initFileList;

signals:
    void processing(bool isProcessing, bool visibleProgress = false);
    void setStatusbarText(const QString &text = QString());
    void folderContentsListCreated(const QString &folderPath, const QList<ExtNumSize> &extList);
}; // class Files

using FileStatus = Files::FileStatus;
using FileStatuses = Files::FileStatuses;
Q_DECLARE_OPERATORS_FOR_FLAGS(Files::FileStatuses)

struct FileValues {
    FileValues(FileStatus initStatus = FileStatus::New) : status(initStatus) {}

    QString checksum; // newly computed or imported from the database
    QString reChecksum; // the recomputed checksum, if it does not match the 'checksum'
    qint64 size = 0; // file size in bytes
    FileStatus status;
}; // struct FileValues

struct ExtNumSize {
    static const QString strNoType;
    static const QString strVeretinoDb;
    static const QString strShaFiles;

    QString extension; // file extension/type, for example: txt (pdf, mkv, 7z, flac...)
    int filesNumber = 0; // number of files with this extension
    qint64 filesSize = 0; // total size of these files
}; // struct ExtNumSize

#endif // FILES_H
