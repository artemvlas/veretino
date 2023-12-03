// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef FILES_H
#define FILES_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QAbstractItemModel>

struct FileValues;
using FileList = QMap<QString, FileValues>; // {relative path to file : FileValues struct}

struct FilterRule {
    enum ExtensionsFilter {NotSet, Include, Ignore};
    void setFilter(const ExtensionsFilter filterType, const QStringList &extensions)
    {
        extensionsList = extensions;
        extensionsList.isEmpty() ? extensionsFilter_ = NotSet : extensionsFilter_ = filterType;
    }

    void clearFilter()
    {
        extensionsFilter_ = NotSet;
        extensionsList.clear();
    }

    bool isFilter(const ExtensionsFilter filterType) const
    {
        return (filterType == extensionsFilter_);
    }

    ExtensionsFilter extensionsFilter_ = NotSet;
    QStringList extensionsList;
    bool ignoreShaFiles = true;
    bool ignoreDbFiles = true;

    FilterRule(bool ignoreSummaries = true) : ignoreShaFiles(ignoreSummaries), ignoreDbFiles(ignoreSummaries) {}
    FilterRule(const ExtensionsFilter filterType, const QStringList &extensions) : extensionsFilter_(filterType), extensionsList(extensions) {}
}; // struct FilterRule

class Files : public QObject
{
    Q_OBJECT
public:
    explicit Files(QObject *parent = nullptr);
    explicit Files(const QString &initPath, QObject *parent = nullptr);
    Files(const FileList &fileList, QObject *parent = nullptr);

    enum FileStatus {
        Undefined,
        Queued, // added to the processing queue
        Processing, // checksum is being calculated
        NotChecked, // available for verification
        Matched, // checked, checksum matched
        Mismatched, // checked, checksum did not match
        New, // a file that is present on disk but not in the database
        Missing, // not on disk, but present in the database
        Unreadable, // present on the disk, but unreadable (no read permission or read error)
        Added, // item (file path and its checksum) has been added to the database
        Removed, // item^ removed from the database
        ChecksumUpdated
    };
    Q_ENUM(FileStatus)

    // functions
    FileList allFiles(); // 'initFolderPath' --> allFiles(const QString &rootFolder)
    FileList allFiles(const QString &rootFolder); // iterate the 'rootFolder', returns all files list
    FileList allFiles(const FilterRule &filter); // return filtered filelist: can ignore or include only files with specified extensions
    FileList allFiles(const QString &rootFolder, const FilterRule &filter);
    FileList allFiles(const FileList &fileList, const FilterRule &filter);

    static bool containsFiles(const QString &folderPath, const FilterRule &filter = FilterRule(false)); // checks whether there are any (or filtered) files the folder/subfolders

    qint64 dataSize(); // total size of all files in the 'initFolderPath' or 'initFileList'
    qint64 dataSize(const QString &folder); // total size of allFiles('folder')
    static qint64 dataSize(const FileList &filelist); // total size of all files in the 'filelist'

    QString contentStatus(const QString &path);
    static QString contentStatus(const FileList &filelist);
    static QString itemInfo(const QAbstractItemModel *model, const QSet<FileStatus> &fileStatuses = QSet<FileStatus>(),
                            const QModelIndex& rootIndex = QModelIndex());

    QString folderContentsByType(const QString &folder);
    static QString folderContentsByType(const FileList &fileList);

    // variables
    bool canceled = false;

public slots:
    void cancelProcess();
    QString contentStatus(); // returns "filename (readable size)" for file, or "folder name: number of files (redable size) for folders"
    QString folderContentsByType(); // returns sorted by data size list of extensions, files number and their size

private:
    // variables
    QString initFilePath; // path to the File specified when creating the object
    QString initFolderPath; // path to the Folder specified when creating the object
    FileList initFileList;

signals:
    void processing(bool isProcessing, bool visibleProgress = false);
    void setStatusbarText(const QString &text = QString());
    void sendText(const QString &text = QString());
}; // class Files

using FileStatus = Files::FileStatus;

struct FileValues {
    FileValues(FileStatus initStatus = FileStatus::New) : status(initStatus) {}

    QString checksum; // newly computed or imported from the database
    QString reChecksum; // the recomputed checksum, if it does not match the 'checksum'
    qint64 size = 0; // file size in bytes
    FileStatus status;
}; // struct FileValues

#endif // FILES_H
