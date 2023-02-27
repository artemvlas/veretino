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

QString Files::fileNameSize(const QString &path)
{
    if (path != nullptr)
        filePath = path;

    QFileInfo fileInfo (filePath);

    return QString("%1 (%2)").arg(fileInfo.fileName(), QLocale(QLocale::English).formattedDataSize(fileInfo.size()));
}

QString Files::filesNumberSizeToReadable(const int &filesNumber, const qint64 &filesSize)
{
    char s = char(); // if only 1 file - text is "file", if more - text is "files"
    if (filesNumber > 1)
        s = 's';

    QString text = QString("%1 file%2 (%3)").arg(filesNumber).arg(s).arg(QLocale(QLocale::English).formattedDataSize(filesSize));

    return text;
}

QString Files::filelistContentStatus(const QStringList &filelist)
{
    int filesNumber = filelist.size();
    qint64 filesSize = filelistSize(filelist);

    return filesNumberSizeToReadable(filesNumber, filesSize);
}

QString Files::folderContentStatus(const QString &folder)
{
    if (folder != nullptr)
        folderPath = folder;

    QStringList filelist = actualFileList();
    QString text = QString("%1: %2").arg(QDir(folderPath).dirName(), filelistContentStatus(filelist));

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

QStringList Files::actualFileListFiltered(const QStringList &extensionsList, const QString &folder)
{
    if (folder != nullptr)
        folderPath = folder;

    if (fileList.isEmpty())
        actualFileList(); // fill 'fileList' with all files in current folder

    QStringList filteredList; //result filelist

    if (ignoreDbFiles || ignoreShaFiles)
        filteredList = filterDbShafiles(fileList);
    else
        filteredList = fileList;

    if (!extensionsList.isEmpty()) {
        filteredList = filterByExtensions(extensionsList, filteredList);
        qDebug() << "Files::actualFileListFiltered | filterByExtensions: " << extensionsList;
    }

    return filteredList;
}

// actual filelist with only listed extensions included
QStringList Files::includedOnlyFilelist(const QStringList &extensionsList, const QString &folder)
{
    if (folder != nullptr)
        folderPath = folder;

    if (fileList.isEmpty())
        actualFileList();

    QStringList resultList; //result filelist

    foreach (const QString &file, fileList) {
        if(extensionsList.contains(QFileInfo(file).suffix().toLower()))
            resultList.append(file);
    }

    return resultList;
}

QStringList& Files::actualFileList(const QString &folder)
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
