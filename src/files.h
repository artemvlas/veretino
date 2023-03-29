#ifndef FILES_H
#define FILES_H

#include <QObject>

// This class is part of the Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
// Objects of this class are used to work with paths and lists of files: join relative paths, create the folder contents lists,
// filtering filelists by file types, return info (conveniently readable if needed) about size, file types, files count, etc.

class Files : public QObject
{
    Q_OBJECT
public:
    explicit Files(QObject *parent = nullptr);
    explicit Files(const QString &initPath, QObject *parent = nullptr);
    Files(const QStringList &fileList, QObject *parent = nullptr);

    // variables
    QString initFilePath; // path to the File specified when creating the object
    QString initFolderPath; // path to the Folder specified when creating the object
    QStringList initFileList;
    QStringList allFilesList; // cached list of all files in initial folder

    // functions
    QStringList& allFiles(); // returns a reference to the 'allFilesList'^; empty list will be filled by overloaded func
    QStringList allFiles(const QString &rootFolder); // iterate the 'rootFolder', returns all files list

    QStringList filteredFileList(const QStringList &extensionsList, const bool includeOnly = false); // return filtered filelist: can ignore or include only files with specified extensions
    QStringList filteredFileList(const QStringList &extensionsList, const QStringList &fileList, const bool includeOnly = false);

    QString parentFolder(); // returns the parent folder of initFilePath;
    static QString parentFolder(const QString &path); // returns the parent folder of the 'path'

    QString joinPath(const QString &addPath); // returns '/initFolderPath/addPath'
    static QString joinPath(const QString &absolutePath, const QString &addPath); // returns '/absolutePath/addPath'

    qint64 dataSize(); // total size of all files in the 'initFolderPath' or 'initFileList'
    qint64 dataSize(const QString &folder); // total size of all files in the 'folder'
    qint64 dataSize(const QStringList &filelist); // total size of all files in the 'filelist'

    static QString dataSizeReadable(const qint64 &sizeBytes);

    static QString fileSize(const QString &filePath); // returns "filename (readable size)" for file, can be used without Object

    QString contentStatus(const QString &path); // returns "filename (readable size)" for file, or "folder name: number of files (redable size) for folders"
    QString contentStatus(const QStringList &filelist);
    QString contentStatus(const int &filesNumber, const qint64 &filesSize);

    QString folderContentsByType(); // returns sorted by data size list of extensions, files number and their size
    QString folderContentsByType(const QString &folder);
    QString folderContentsByType(const QStringList &fileList);

public slots:
    void cancelProcess();
    QString contentStatus();

private:
    // variables
    bool canceled = false;

signals:
    void sendText(const QString &text);
};

#endif // FILES_H
