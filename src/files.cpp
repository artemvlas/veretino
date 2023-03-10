#include "files.h"
#include "QDirIterator"
#include "QThread"
#include "jsondb.h"
#include "treemodel.h"

Files::Files(const QString &initPath, QObject *parent)
    : QObject{parent}
{
    if (initPath != nullptr) {
        if (QFileInfo(initPath).isFile())
            initFilePath = initPath;
        else if (QFileInfo(initPath).isDir())
            initFolderPath = initPath;
    }
}

QString Files::fileNameSize(const QString &path)
{
    QString pathToFile;
    if (path != nullptr)
        pathToFile = path;
    else
        pathToFile = initFilePath;

    QFileInfo fileInfo (pathToFile);

    return QString("%1 (%2)").arg(fileInfo.fileName(), QLocale(QLocale::English).formattedDataSize(fileInfo.size()));
}

QString Files::filesNumberSizeToReadable(const int &filesNumber, const qint64 &filesSize)
{
    char s = char(); // if only 1 file - text is "file", if more - text is "files"
    if (filesNumber != 1)
        s = 's';

    return QString("%1 file%2 (%3)").arg(filesNumber).arg(s).arg(QLocale(QLocale::English).formattedDataSize(filesSize));
}

QString Files::filelistContentStatus(const QStringList &filelist)
{
    return filesNumberSizeToReadable(filelist.size(), filelistDataSize(filelist));
}

QString Files::folderContentStatus(const QString &folder)
{
    return QString("%1: %2").arg(QDir(initFolderPath).dirName(), filelistContentStatus(allFiles(folder)));
}

QString Files::folderContentsByType(const QString &folder)
{
    QStringList files = allFiles(folder);

    QString text;

    if (files.isEmpty())
        return "Empty folder";

    QHash<QString, QStringList> listsByType; // key = extension, value = list of files with that extension

    foreach (const QString &file, files) {
        QString ext = QFileInfo(file).suffix().toLower();
        if (ext == "")
            ext = "No type";
        listsByType[ext].append(file);
    }

    struct combinedByType {
        QString extension;
        QStringList filelist;
        qint64 filesSize;
    };

    QList<combinedByType> combList;

    foreach (const QString &ext, listsByType.keys()) {
        combinedByType t;
        t.extension = ext;
        t.filelist = listsByType.value(ext);
        t.filesSize = filelistDataSize(t.filelist);
        combList.append(t);
    }

    std::sort(combList.begin(), combList.end(), [](const combinedByType &t1, const combinedByType &t2) {return (t1.filesSize > t2.filesSize);});

    if (combList.size() > 10) {
        text.append(QString("Top sized file types:\n%1\n").arg(QString('-').repeated(30)));
        qint64 excSize = 0; // total size of files whose types are not displayed
        int excNumber = 0; // the number of these files
        for (int var = 0; var < combList.size(); ++var) {
            if (var < 10) {
                text.append(QString("%1: %2\n").arg(combList.at(var).extension, filelistContentStatus(combList.at(var).filelist)));
            }
            else {
                excSize += combList.at(var).filesSize;
                excNumber += combList.at(var).filelist.size();
            }
        }
        text.append(QString("...\nOther %1 types: %2\n").arg(combList.size() - 10).arg(filesNumberSizeToReadable(excNumber, excSize)));
    }
    else {
        foreach (const combinedByType &t, combList) {
            text.append(QString("%1: %2\n").arg(t.extension, filelistContentStatus(t.filelist)));
        }
    }

    QString totalInfo = QString(" Total: %1 types, %2").arg(combList.size()).arg(filelistContentStatus(files));
    text.append(QString("%1\n%2").arg(QString('_').repeated(totalInfo.length()), totalInfo));

    return text;
}

QStringList Files::filteredFileList(QStringList extensionsList, const bool includeOnly, const QStringList &filelist)
{
    QStringList files;
    QStringList filteredFiles; // result list

    if (filelist != QStringList())
        files = filelist;
    else
        files = allFiles();

    if (!includeOnly && ignoreDbFiles)
        extensionsList.append("ver.json");
    if (!includeOnly && ignoreShaFiles)
        extensionsList.append({"sha1", "sha256", "sha512"});

    if (extensionsList.isEmpty()) {
        qDebug() << "Files::filteredFileList | 'extensionsList' is Empty. Original list returned";
        return files;
    }

    foreach (const QString &file, files) {
        // to be able to filter compound extensions (like *.ver.json), a comparison loop is used instead of
        // 'extensionsList.contains(QFileInfo(file).suffix().toLower())'
        bool allowed = !includeOnly;
        foreach (const QString &ext, extensionsList) {
            if (file.endsWith('.' + ext, Qt::CaseInsensitive)) {
                allowed = includeOnly;
                break;
            }
        }
        if (allowed)
            filteredFiles.append(file);
    }

    return filteredFiles;
}

QStringList& Files::allFiles(const QString &rootFolder)
{
    if (rootFolder != nullptr) {
        allFilesListCustomFolder = iterateFolder(rootFolder);
        return allFilesListCustomFolder;
    }
    else {
        if (allFilesList.isEmpty())
            allFilesList = iterateFolder(initFolderPath);
        return allFilesList;
    }
}

QStringList Files::iterateFolder(const QString &rootFolder)
{
    QStringList fileList;
    QDirIterator it(rootFolder, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        fileList.append(it.next());
    return fileList;
}

qint64 Files::filelistDataSize(const QStringList &filelist)
{
    qint64 size = 0;

    foreach (const QString &file, filelist) {
        size += QFileInfo(file).size();
    }

    return size;
}

int Files::filesNumber(const QString &folder)
{ 
    return allFiles(folder).size();
}

qint64 Files::folderSize(const QString &folder)
{   
    return filelistDataSize(allFiles(folder));
}
