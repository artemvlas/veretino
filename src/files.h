#ifndef FILES_H
#define FILES_H

#include <QObject>

// Objects of this class are used to work with lists of files. Folder contents lists, info about size, file types, files count, etc.
// As well as the output of conveniently readable information about files and folders.

class Files : public QObject
{
    Q_OBJECT
public:
    explicit Files(const QString &initPath = QString(), QObject *parent = nullptr);
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
    int filesNumber(const QString &folder = QString());
    qint64 filelistDataSize(const QStringList &filelist); // total size of all files in the 'filelist'
    qint64 folderSize(const QString &folder = QString()); // total size of all files in the 'folder'
    QString dataSizeReadable(const qint64 &sizeBytes);
    QString folderContentStatus(const QString &folder = QString());
    QString filelistContentStatus(const QStringList &filelist);
    QString filesNumberSizeToReadable(const int &filesNumber, const qint64 &filesSize);
    QString fileNameSize(const QString &path = QString()); // returns "filename (readable size)"
    QString folderContentsByType(const QString &folder = QString()); // returns sorted by data size list of extensions, files number and their size

private:
    QStringList iterateFolder(const QString &rootFolder); // returns a list of all files in specified folder and all its subfolders
};

#endif // FILES_H
