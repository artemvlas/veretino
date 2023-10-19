// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#include "files.h"
#include <QDirIterator>
#include <QDebug>
#include "tools.h"

Files::Files(QObject *parent)
    : QObject(parent)
{}

Files::Files(const QString &initPath, QObject *parent)
    : QObject(parent)
{
    if (QFileInfo(initPath).isFile())
        initFilePath = initPath;
    else if (QFileInfo(initPath).isDir())
        initFolderPath = initPath;
}

Files::Files(const FileList &fileList, QObject *parent)
    : QObject(parent), initFileList(fileList)
{}

FileList Files::allFiles()
{
    return allFiles(initFolderPath);
}

FileList Files::allFiles(const QString &rootFolder)
{
    return allFiles(rootFolder, FilterRule());
}

FileList Files::allFiles(const FilterRule &filter)
{
    return allFiles(initFolderPath, filter);
}

FileList Files::allFiles(const QString &rootFolder, const FilterRule &filter)
{
    if (!QFileInfo(rootFolder).isDir()) {
        qDebug() << "Files::allFiles | Not a folder path: " << rootFolder;
        return FileList();
    }

    emit setStatusbarText("Creating a list of files...");

    canceled = false;
    FileList resultList; // result list

    QDir dir(rootFolder);
    QDirIterator it(rootFolder, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !canceled) {
        QString fullPath = it.next();
        QString relPath = dir.relativeFilePath(fullPath);

        if (paths::isFileAllowed(relPath, filter)) {
            FileValues curFileValues;
            QFileInfo fileInfo(fullPath);
            if (fileInfo.isReadable())
                curFileValues.size = fileInfo.size(); // If the file is unreadable, then its size is not needed
            else
                curFileValues.status = FileValues::Unreadable;

            resultList.insert(relPath, curFileValues);
        }
    }

    if (canceled) {
        qDebug() << "Files::allFiles | Canceled:" << rootFolder;
        emit setStatusbarText();
        return FileList();
    }

    emit setStatusbarText(QString("%1 files found").arg(resultList.size()));
    return resultList;
}

FileList Files::allFiles(const FileList &fileList, const FilterRule &filter)
{
    FileList filteredFiles; // result list
    FileList::const_iterator iter;

    for (iter = fileList.constBegin(); iter != fileList.constEnd(); ++iter) {
        if (paths::isFileAllowed(iter.key(), filter))
            filteredFiles.insert(iter.key(), iter.value());
    }

    return filteredFiles;
}

QString Files::contentStatus()
{
    if (!initFilePath.isEmpty())
        return contentStatus(initFilePath);
    else if (!initFolderPath.isEmpty())
        return contentStatus(initFolderPath);
    else if (!initFileList.isEmpty())
        return contentStatus(initFileList);
    else
        return "Files::contentStatus() | No content to display status";
}

QString Files::contentStatus(const QString &path)
{
    QFileInfo fileInfo(path);
    QString result;

    if (fileInfo.isDir()) {
        canceled = false;
        int filesNumber = 0;
        qint64 totalSize = 0;
        QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);

        emit setStatusbarText("counting...");

        while (it.hasNext() && !canceled) {
            totalSize += QFileInfo(it.next()).size();
            ++filesNumber;
        }

        if (!canceled) {
            result = QString("%1: %2").arg(paths::folderName(path), format::filesNumberAndSize(filesNumber, totalSize));
        }
        else {
            qDebug()<< "Files::contentStatus(const QString &path) | Canceled" << path;
        }
    }
    else if (fileInfo.isFile()) {
        result = format::fileNameAndSize(path);
    }
    else
        qDebug() << "Files::contentStatus(const QString &path) | The 'path' doesn't exist";

    emit setStatusbarText(result);
    return result;
}

QString Files::contentStatus(const FileList &fileList)
{
    return format::filesNumberAndSize(fileList.size(), dataSize(fileList));
}

QString Files::folderContentsByType()
{
    if (!initFolderPath.isEmpty()) {
        FilterRule filter;
        filter.ignoreDbFiles = false;
        filter.ignoreShaFiles = false;
        QString result = folderContentsByType(allFiles(filter));

        if (canceled)
            result.clear();

        emit sendText(result);
        return result;
    }
    else if (!initFileList.isEmpty())
        return folderContentsByType(initFileList);
    else
        return "Files::folderContentsByType | No initial data";
}

QString Files::folderContentsByType(const QString &folder)
{
    return folderContentsByType(allFiles(folder));
}

QString Files::folderContentsByType(const FileList &fileList)
{
    if (fileList.isEmpty())
        return "Empty folder. No file types to display.";

    QHash<QString, FileList> listsByType; // key = extension, value = list of files with that extension

    FileList::const_iterator filesIter;
    for (filesIter = fileList.constBegin(); filesIter != fileList.constEnd(); ++filesIter) {
        QString ext = QFileInfo(filesIter.key()).suffix().toLower();
        if (ext.isEmpty())
            ext = "No type";
        listsByType[ext].insert(filesIter.key(), filesIter.value());
    }

    struct combinedByType {
        QString extension;
        FileList filelist;
        qint64 filesSize;
    };

    QList<combinedByType> combList;

    QHash<QString, FileList>::const_iterator iter;
    for (iter = listsByType.constBegin(); iter != listsByType.constEnd(); ++iter) {
        combinedByType t;
        t.extension = iter.key();
        t.filelist = iter.value();
        t.filesSize = dataSize(t.filelist);
        combList.append(t);
    }

    std::sort(combList.begin(), combList.end(), [](const combinedByType &t1, const combinedByType &t2) {return (t1.filesSize > t2.filesSize);});

    QString text;
    if (combList.size() > 10) {
        text.append(QString("Top sized file types:\n%1\n").arg(QString('-').repeated(30)));
        qint64 excSize = 0; // total size of files whose types are not displayed
        int excNumber = 0; // the number of these files
        for (int var = 0; var < combList.size(); ++var) {
            if (var < 10) {
                text.append(QString("%1: %2\n").arg(combList.at(var).extension, contentStatus(combList.at(var).filelist)));
            }
            else {
                excSize += combList.at(var).filesSize;
                excNumber += combList.at(var).filelist.size();
            }
        }
        text.append(QString("...\nOther %1 types: %2\n").arg(combList.size() - 10).arg(format::filesNumberAndSize(excNumber, excSize)));
    }
    else {
        foreach (const combinedByType &t, combList) {
            text.append(QString("%1: %2\n").arg(t.extension, contentStatus(t.filelist)));
        }
    }

    QString totalInfo = QString(" Total: %1 types, %2").arg(combList.size()).arg(contentStatus(fileList));
    text.append(QString("%1\n%2").arg(QString('_').repeated(totalInfo.length()), totalInfo));

    return text;
}

qint64 Files::dataSize()
{
    if (!initFolderPath.isEmpty())
        return dataSize(allFiles());
    else if (!initFileList.isEmpty())
        return dataSize(initFileList);
    else {
        qDebug() << "Files::dataSize() | No data to size return";
        return 0;
    }
}

qint64 Files::dataSize(const QString &folder)
{
    return dataSize(allFiles(folder));
}

qint64 Files::dataSize(const FileList &filelist)
{
    qint64 totalSize = 0;

    if (!filelist.isEmpty()) {
        FileList::const_iterator iter;
        for (iter = filelist.constBegin(); iter != filelist.constEnd(); ++iter) {
            totalSize += iter.value().size;
        }
    }

    return totalSize;
}

void Files::cancelProcess()
{
    canceled = true;
}

namespace paths {
QString folderName(const QString &folderPath)
{
    QString dirName = QDir(folderPath).dirName();

    if (dirName.isEmpty()) {
        QString rootPath = parentFolder(folderPath);
        if (rootPath.size() == 3 && rootPath.at(1) == ':') // if Windows-style root path like C:
            dirName = QString("Drive_%1").arg(rootPath.at(0));
        else
            dirName = "Root";
    }

    return dirName;
}

QString parentFolder(const QString &path)
{
    int rootSepIndex = path.indexOf('/'); // index of root '/': 0 for '/home/folder'; 2 for 'C:/folder'
    if (rootSepIndex == -1)
        return "/"; // if there is no '/' in 'path'

    if (path.at(rootSepIndex + 1) == '/')
        ++rootSepIndex; // if the path's root contains double '/' like 'ftp://folder' or 'smb://folder', increase index to next position
    int sepIndex = path.lastIndexOf('/', -2); // skip the last char due the case /home/folder'/'
    if (sepIndex > rootSepIndex)
        return path.left(sepIndex);
    else
        return path.left(rootSepIndex + 1); // if the last 'sep' is also the root, keep it
}

QString joinPath(const QString &absolutePath, const QString &addPath)
{
    if (absolutePath.endsWith('/'))
        return absolutePath + addPath;
    else
        return QString("%1/%2").arg(absolutePath, addPath);
}

QString backupFilePath(const QString &filePath)
{
    return joinPath(parentFolder(filePath), ".tmp-backup_" + QFileInfo(filePath).fileName());
}

bool isFileAllowed(const QString &filePath, const FilterRule &filter)
{
    if (!filter.includeOnly) {
        if (tools::isDatabaseFile(filePath))
            return !filter.ignoreDbFiles;
        if (tools::isSummaryFile(filePath))
            return !filter.ignoreShaFiles;
    }

    if (filter.extensionsList.isEmpty())
        return true;

    // if 'filter.include' = true, a file ('filePath') with any extension from 'extensionsList' is allowed
    // if 'filter.include' = false, than all files except these types allowed

    bool allowed = !filter.includeOnly;
    foreach (const QString &ext, filter.extensionsList) {
        if (filePath.endsWith('.' + ext, Qt::CaseInsensitive)) {
            allowed = filter.includeOnly;
            break;
        }
    }

    return allowed;
}
} // namespace 'paths'
