#ifndef FILES_H
#define FILES_H

#include <QObject>

// This class is part of the Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
// Objects of this class are used to work with lists of files. Folder contents lists, info about size, file types, files count, etc.
// As well as the output of conveniently readable information about files and folders.

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
    bool ignoreDbFiles = true; // exlude *.ver.json files while filtering
    bool ignoreShaFiles = true; // exclude *.sha1/256/512 files while filtering
    QStringList allFilesList; // cached list of all files in initial folder
    QStringList allFilesListCustomFolder; // cached list of all files in specified as argument folder

    // functions
    QStringList& allFiles(const QString &rootFolder = QString()); // calls 'iterateFolder' if needed and returns a reference to the 'allFilesList'^ or 'allFilesListCustomFolder'^
    QStringList filteredFileList(QStringList extensionsList, const bool includeOnly = false, const QStringList &filelist = QStringList());

    QString parentFolder(); // returns the parent folder of initFilePath;
    static QString parentFolder(const QString &path); // returns the parent folder of the 'path'

    QString joinPath(const QString &addPath); // returns 'initFolderPath/addPath'
    static QString joinPath(const QString &absolutePath, const QString &addPath); // returns 'absolutePath/addPath'

    qint64 dataSize(); // total size of all files in the 'initFolderPath' or 'initFileList'
    qint64 dataSize(const QString &folder); // total size of all files in the 'folder'
    qint64 dataSize(const QStringList &filelist); // total size of all files in the 'filelist'

    static QString dataSizeReadable(const qint64 &sizeBytes);

    QString contentStatus();
    QString contentStatus(const QString &path); // returns "filename (readable size)" for file, or "folder name: number of files (redable size) for folders"
    QString contentStatus(const QStringList &filelist);
    QString contentStatus(const int &filesNumber, const qint64 &filesSize);

    QString folderContentsByType(const QString &folder = QString()); // returns sorted by data size list of extensions, files number and their size

private:
    QStringList iterateFolder(const QString &rootFolder); // returns a list of all files in specified folder and all its subfolders
};

#endif // FILES_H
