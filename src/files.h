/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef FILES_H
#define FILES_H

#include <QObject>
#include <QMap>
#include <QAbstractItemModel>
#include "filterrule.h"
#include "procstate.h"

struct FileValues;
using FileList = QMap<QString, FileValues>;    // {relative path to file : FileValues struct}

struct FileTypeList {
    QHash<QString, NumSize> extensions;        // {file extension (suffix) : files number and size}
    QHash<FilterAttribute, NumSize> combined;  // {combined type like 'SymLinks' or 'UnPermitted' : files number and size}

    int count() const { return extensions.size() + combined.size(); }
    bool isEmpty() const { return count() == 0; }
    explicit operator bool() const { return !isEmpty(); }
}; // struct FileTypeList

class Files : public QObject
{
    Q_OBJECT

public:
    explicit Files(QObject *parent = nullptr);
    explicit Files(const QString &path, QObject *parent = nullptr);

    enum FileStatus {
        NotSet = 0,

        Queued = 1 << 0,        // added to the processing queue
        Calculating = 1 << 1,   // checksum is being calculated
        Verifying = 1 << 2,     // checksum is being verified
        CombProcessing = Queued | Calculating | Verifying,

        NotChecked = 1 << 3,    // available for verification
        NotCheckedMod = 1 << 4, // same, but the file modif. time was checked and it is later than the db creation
        Matched = 1 << 5,       // checked, checksum matched
        Mismatched = 1 << 6,    // checked, checksum did not match
        New = 1 << 7,           // a file that is present on disk but not in the database
        Missing = 1 << 8,       // not on disk, but present in the database
        Added = 1 << 9,         // item (file path and its checksum) has been added to the database
        Removed = 1 << 10,      // item^ removed from the database
        Updated = 1 << 11,      // the checksum has been updated
        Imported = 1 << 12,     // the checksum was imported from another file
        Moved = 1 << 13,        // the newly calculated checksum corresponds to some missing item (renamed or moved file)
        MovedOut = 1 << 14,     // former Missing when moving
        UnPermitted = 1 << 15,  // no read permissions
        ReadError = 1 << 16,    // an error occurred during reading

        CombNotChecked = NotChecked | NotCheckedMod | Imported,
        CombAvailable = CombNotChecked | Matched | Mismatched | Added | Updated | Moved,
        CombHasChecksum = CombAvailable | Missing,
        CombUpdatable = New | Missing | Mismatched,
        CombDbChanged = Added | Removed | Updated | Imported | Moved,
        CombChecked = Matched | Mismatched,
        CombMatched = Matched | Added | Updated | Moved,
        CombNewLost = New | Missing,
        CombUnreadable = UnPermitted | ReadError,
    }; // enum FileStatus

    Q_ENUM(FileStatus)
    Q_DECLARE_FLAGS(FileStatuses, FileStatus)

    // functions
    void setProcState(const ProcState *procState);
    void setPath(const QString &path);                    // path to file or folder >> 'fsPath_'
    FileList getFileList();                               // 'fsPath_' --> getFileList(const QString &rootFolder)
    FileList getFileList(const FilterRule &filter);       // return filtered filelist: can ignore or include only files with specified extensions
    FileList getFileList(const QString &rootFolder,
                         const FilterRule &filter = FilterRule());
    FileList getFileList(const QAbstractItemModel *model,
                         const FileStatuses flag,
                         const QModelIndex &rootIndex = QModelIndex());

    qint64 dataSize();                                     // total size of all files in the 'fsPath_' folder
    qint64 dataSize(const QString &folder);                // total size of getFileList('folder')
    static qint64 dataSize(const FileList &filelist);      // total size of all files in the 'filelist'

    QString getFolderSize();                               // returns "folder name: number of files (redable size)"
    QString getFolderSize(const QString &path);

    // returns a list of file types (extensions) with files number and their size
    FileTypeList getFileTypes(const QString &folderPath, FilterRule combine); // FilterAttributes are used to combine types
    FileTypeList getFileTypes(const QAbstractItemModel *model,
                              const QModelIndex &rootIndex = QModelIndex());

    static NumSize totalListed(const FileTypeList &_typeList);

    // checks whether there are any (or filtered) files the folder/subfolders
    static bool isEmptyFolder(const QString &folderPath,
                              const FilterRule &filter = FilterRule(FilterAttribute::NoAttributes));
    static QString firstDbFile(const QString &folderPath); // returns full path
    static QStringList dbFiles(const QString &folderPath); // file names only

private:
    bool isCanceled() const;

    // variables
    QString m_fsPath; // path to the File or Folder specified when creating the object
    const ProcState *m_proc = nullptr;

signals:
    void setStatusbarText(const QString &text = QString());
}; // class Files

using FileStatus = Files::FileStatus;
using FileStatuses = Files::FileStatuses;
Q_DECLARE_OPERATORS_FOR_FLAGS(Files::FileStatuses)

struct FileValues {
    FileValues(FileStatus fileStatus = FileStatus::NotSet, qint64 fileSize = -1)
        : status(fileStatus), size(fileSize) {}

    enum HashingPurpose : quint8 {
        Generic,           // the checksum has been calculated and is ready for further processing (copy or save)
        AddToDb,
        Verify,
        CopyToClipboard,   // the calculated checksum is intended to be copied to the clipboard
        SaveToDigestFile   // ... stored in a summary file
    };

    /*** Variables ***/
    FileStatus status = FileStatus::NotSet;
    HashingPurpose hash_purpose = Generic;

    qint64 hash_time = -1;    // hashing time in milliseconds, -1 if not set
    qint64 size = -1;         // file size in bytes, -1 if not set
    QString checksum;         // newly computed or imported from the database
    QString reChecksum;       // the recomputed checksum, if it does not match the 'checksum'
}; // struct FileValues

#endif // FILES_H
