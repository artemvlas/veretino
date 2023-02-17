#include "files.h"
#include "QDirIterator"
#include "QThread"
#include "jsondb.h"
#include "treemodel.h"

Files::Files(const QString &path, QObject *parent)
    : QObject{parent}
{
    if (path != nullptr) {
        if (QFileInfo(path).isFile())
            filePath = path;
        else if (QFileInfo(path).isDir())
            folderPath = path;
    }
    ignoreDbFiles = true;
    ignoreShaFiles = true;
}

QString Files::folderContentStatus(const QString &folder)
{
    if (folder != nullptr)
        folderPath = folder;
    QStringList filelist = actualFileList(folderPath);
    qint64 folderSize = filelistSize(filelist);
    QString text = QString("%1: %2 files * %3").arg(QDir(folderPath).dirName()).arg(filelist.size()).arg(QLocale().formattedDataSize(folderSize));

    return text;
}

// filtering *.ver.json or/and *.sha1/256/512 files from filelist
QStringList Files::filterDbShafiles(const QStringList &filelist)
{
    QStringList resultList; 

    foreach(const QString &i, filelist) {
        if (ignoreDbFiles) {
            if(i.endsWith(".ver.json"))
                continue;
        }
        if (ignoreShaFiles) {
            if (i.endsWith("sha1") || i.endsWith("sha256") || i.endsWith("sha512"))
                continue;
        }
        resultList.append(i);
    }

    return resultList;
}

QStringList Files::filterByExtensions(const QStringList &extensionsList, const QStringList &filelist)
{
    if (filelist != QStringList())
        fileList = filelist;

    foreach (const QString &file, fileList) {
        if(!extensionsList.contains(QFileInfo(file).suffix().toLower()))
            filteredFiles.append(file);
    }

    return filteredFiles;
}

QStringList Files::actualFileListFiltered(const QString &folder, const QStringList &extensionsList)
{
    if (folder != nullptr)
        folderPath = folder;

    if (fileList.isEmpty())
        actualFileList();

    QStringList actualFiles = fileList; // all files in current folder

    QStringList filteredList; //result filelist

    if (ignoreDbFiles || ignoreShaFiles)
        filteredList = filterDbShafiles(actualFiles);
    else
        filteredList = actualFiles;

    if (!extensionsList.isEmpty()) {
        filteredList = filterByExtensions(extensionsList, filteredList);
        qDebug() << "Files::actualFileListFiltered | filterByExtensions: " << extensionsList;
    }

    return filteredList;
}

QStringList Files::actualFileList(const QString &folder)
{
    if (folder != nullptr)
        folderPath = folder;

    QDirIterator it(folderPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        fileList.append(it.next());

    //emit completeList(fileList);
    return fileList;
}

qint64 Files::filelistSize(const QStringList &filelist)
{
    qint64 size = 0;

    foreach (const QString &file, filelist) {
        size += QFileInfo(file).size();
    }

    //emit totalSize(size);
    return size;
}

void Files::processFileList(const QString &rootFolder)
{
    filelistSize(actualFileList(rootFolder));
}

int Files::filesNumber(const QString &folder)
{
    if (folder != nullptr)
        folderPath = folder;
    return actualFileList(folderPath).size();
}

qint64 Files::folderSize(const QString &folder)
{
    if (folder != nullptr)
        folderPath = folder;
    return filelistSize(actualFileList(folderPath));
}
