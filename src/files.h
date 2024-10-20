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
struct NumSize;
using FileList = QMap<QString, FileValues>; // {relative path to file : FileValues struct}
using FileTypeList = QHash<QString, NumSize>;

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
        UnPermitted = 1 << 12,  // no read permissions
        ReadError = 1 << 13,    // an error occurred during reading

        CombNotChecked = NotChecked | NotCheckedMod,
        CombAvailable = CombNotChecked | Matched | Mismatched | Added | Updated,
        CombHasChecksum = CombAvailable | Missing,
        CombUpdatable = New | Missing | Mismatched,
        CombDbChanged = Added | Removed | Updated,
        CombChecked = Matched | Mismatched,
        CombMatched = Matched | Added | Updated,
        CombNewLost = New | Missing,
        CombUnreadable = UnPermitted | ReadError, // present on the disk, but unreadable (no read permission or read error)

        Computed = 1 << 14,    // the checksum has been calculated and is ready for further processing (copy or save)
        ToClipboard = 1 << 15, // the calculated checksum is intended to be copied to the clipboard
        ToSumFile = 1 << 16,   // the calculated checksum is intended to be stored in the summary file
    };
    Q_ENUM(FileStatus)
    Q_DECLARE_FLAGS(FileStatuses, FileStatus)

    // functions
    void setProcState(const ProcState *procState);
    void setPath(const QString &path); // path to file or folder >> 'fsPath_'
    FileList getFileList(); // 'fsPath_' --> getFileList(const QString &rootFolder)
    FileList getFileList(const FilterRule &filter); // return filtered filelist: can ignore or include only files with specified extensions
    FileList getFileList(const QString &rootFolder, const FilterRule &filter = FilterRule());
    FileList getFileList(const QAbstractItemModel *model, const FileStatuses flag, const QModelIndex &rootIndex = QModelIndex());

    qint64 dataSize(); // total size of all files in the 'fsPath_' folder
    qint64 dataSize(const QString &folder); // total size of getFileList('folder')
    static qint64 dataSize(const FileList &filelist); // total size of all files in the 'filelist'

    QString getFolderSize(); // returns "folder name: number of files (redable size)"
    QString getFolderSize(const QString &path);

    // returns a list of file types (extensions) with files number and their size
    FileTypeList getFileTypes(const QString &folderPath, bool excludeUnPerm = false);
    FileTypeList getFileTypes(const QAbstractItemModel *model, const QModelIndex &rootIndex = QModelIndex());

    static NumSize totalListed(const FileTypeList &_typeList);
    static QString suffixName(const QString &_file);

    // checks whether there are any (or filtered) files the folder/subfolders
    static bool isEmptyFolder(const QString &folderPath, const FilterRule &filter = FilterRule(false));
    static QString firstDbFile(const QString &folderPath); // returns full path
    static QStringList dbFiles(const QString &folderPath); // file names only

    static const QString desktopFolderPath; // path to the user's Desktop folder

    static const QString strNoType;
    static const QString strVeretinoDb;
    static const QString strShaFiles;
    static const QString strNoPerm;

private:
    bool isCanceled() const;

    // variables
    QString fsPath_; // path to the File or Folder specified when creating the object
    const ProcState *proc_ = nullptr;

signals:
    void setStatusbarText(const QString &text = QString());
}; // class Files

using FileStatus = Files::FileStatus;
using FileStatuses = Files::FileStatuses;
Q_DECLARE_OPERATORS_FOR_FLAGS(Files::FileStatuses)

struct FileValues {
    FileValues(FileStatus fileStatus = FileStatus::NotSet, qint64 fileSize = -1)
        : status(fileStatus), size(fileSize) {}

    FileStatus status;
    qint64 size; // file size in bytes
    QString checksum; // newly computed or imported from the database
    QString reChecksum; // the recomputed checksum, if it does not match the 'checksum'
}; // struct FileValues

struct NumSize { // number and total size (of files)
    NumSize() {}
    NumSize(int num, qint64 size) : _num(num), _size(size) {}
    void add(int num, qint64 size) { _num += num; _size += size; }
    void add(const NumSize &other) { add(other._num, other._size); }
    void addOne(qint64 size = 0) { ++_num; _size += size; }
    void subtract(int num, qint64 size) { _num -= num; _size -= size; }
    void subtract(const NumSize &other) { subtract(other._num, other._size); }
    void subtractOne(qint64 size = 0) { --_num; _size -= size; }
    NumSize& operator<<(qint64 size) { addOne(size); return *this; }
    NumSize& operator<<(const NumSize &other) { add(other); return *this; }
    NumSize& operator+=(const NumSize &other) { add(other); return *this; }
    NumSize& operator-=(const NumSize &other) { subtract(other); return *this; }
    NumSize& operator-=(qint64 size) { subtractOne(size); return *this; }
    NumSize& operator++() { ++_num; return *this; } // prefix
    friend NumSize operator+(NumSize lhs, const NumSize& rhs) { lhs += rhs; return lhs; }
    friend NumSize operator-(NumSize lhs, const NumSize& rhs) { lhs -= rhs; return lhs; }
    explicit operator bool() const { return _num > 0; }

    // values
    int _num = 0;
    qint64 _size = 0;
}; // struct NumSize

#endif // FILES_H
