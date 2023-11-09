// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef FILES_H
#define FILES_H

#include <QObject>
#include <QMap>
#include <QSet>
#include <QAbstractItemModel>

struct FileValues {
    QString checksum; // newly computed or imported from the database
    QString reChecksum; // the recomputed checksum, if it does not match the 'checksum'
    qint64 size = 0; // file size in bytes

    enum FileStatus {
        Listed, // added to filelist during folder iteration
        Imported, // imported when parsing the database
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

    FileStatus status = NotChecked;
}; // struct FileValues

using FileList = QMap<QString, FileValues>; // {relative path to file : FileValues struct}

struct FilterRule {
    QStringList extensionsList;
    bool includeOnly = false; // if true, only files with any extension from the list included, else all files except these types
    bool ignoreShaFiles = true;
    bool ignoreDbFiles = true;
}; // struct FilterRule

class Files : public QObject
{
    Q_OBJECT
public:
    explicit Files(QObject *parent = nullptr);
    explicit Files(const QString &initPath, QObject *parent = nullptr);
    Files(const FileList &fileList, QObject *parent = nullptr);

    // functions
    FileList allFiles(); // 'initFolderPath' --> allFiles(const QString &rootFolder)
    FileList allFiles(const QString &rootFolder); // iterate the 'rootFolder', returns all files list
    FileList allFiles(const FilterRule &filter); // return filtered filelist: can ignore or include only files with specified extensions
    FileList allFiles(const QString &rootFolder, const FilterRule &filter);
    FileList allFiles(const FileList &fileList, const FilterRule &filter);

    qint64 dataSize(); // total size of all files in the 'initFolderPath' or 'initFileList'
    qint64 dataSize(const QString &folder); // total size of allFiles('folder')
    static qint64 dataSize(const FileList &filelist); // total size of all files in the 'filelist'
    static qint64 dataSize(const QAbstractItemModel* model, const QSet<FileValues::FileStatus>& fileStatuses = QSet<FileValues::FileStatus>(),
                                                                                                const QModelIndex& rootIndex = QModelIndex());

    QString contentStatus(const QString &path);
    static QString contentStatus(const FileList &filelist);

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
    void setStatusbarText(const QString &text = QString());
    void sendText(const QString &text = QString());
};

#endif // FILES_H
