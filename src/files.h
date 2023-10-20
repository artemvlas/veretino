// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef FILES_H
#define FILES_H

#include <QObject>
#include <QMap>

struct FileValues {
    QString checksum; // newly computed or imported from the database
    QString reChecksum; // the recomputed checksum, if it does not match the 'checksum'
    qint64 size = 0; // file size in bytes
    enum FileStatus {NotChecked, Matched, Mismatched, New, Missing, Unreadable, Added, Removed, ChecksumUpdated};
    FileStatus status = NotChecked;
}; // struct FileValues

using FileList = QMap<QString, FileValues>; // {relative path to file : FileValues struct}

struct FilterRule {
    QStringList extensionsList;
    bool includeOnly = false; // if true, only files with any extension from the list included, else all files except these types
    bool ignoreShaFiles = true;
    bool ignoreDbFiles = true;
}; // struct FilterRule

namespace paths {
QString parentFolder(const QString &path); // returns the parent folder of the 'path'
QString folderName(const QString &folderPath); // returns folder name: "/home/user/folder" --> "folder"; if empty, returns 'Root'
QString basicName(const QString &path); // returns file or folder name: "/home/user/folder/fname" --> "fname"
QString joinPath(const QString &absolutePath, const QString &addPath); // returns '/absolutePath/addPath'
QString backupFilePath(const QString &filePath);

bool isFileAllowed(const QString &filePath, const FilterRule &filter); // whether the file extension matches the filter rules
} // namespace paths

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
