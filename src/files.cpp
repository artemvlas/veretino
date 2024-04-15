/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#include "files.h"
#include <QDirIterator>
#include <QDebug>
#include "tools.h"
#include "treemodeliterator.h"
#include "treemodel.h"

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

FileList Files::getFileList()
{
    return getFileList(initFolderPath);
}

FileList Files::getFileList(const QString &rootFolder)
{
    return getFileList(rootFolder, FilterRule());
}

FileList Files::getFileList(const FilterRule &filter)
{
    return getFileList(initFolderPath, filter);
}

FileList Files::getFileList(const QString &rootFolder, const FilterRule &filter)
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

        if (filter.isFileAllowed(relPath)) {
            FileValues curFileValues;
            QFileInfo fileInfo(fullPath);
            if (fileInfo.isReadable())
                curFileValues.size = fileInfo.size(); // If the file is unreadable, then its size is not needed
            else
                curFileValues.status = Files::Unreadable;

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

FileList Files::getFileList(const FileList &fileList, const FilterRule &filter)
{
    FileList filteredFiles; // result list
    FileList::const_iterator iter;

    for (iter = fileList.constBegin(); iter != fileList.constEnd(); ++iter) {
        if (filter.isFileAllowed(iter.key()))
            filteredFiles.insert(iter.key(), iter.value());
    }

    return filteredFiles;
}

bool Files::isEmptyFolder(const QString &folderPath, const FilterRule &filter)
{
    bool result = true;

    if (QFileInfo(folderPath).isDir()) {
        QDirIterator it(folderPath, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            if (filter.isFileAllowed(it.next())) {
                result = false;
                break;
            }
        }
    }
    else
        qDebug() << "Files::containsFiles | Not a folder path: " << folderPath;

    return result;
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
            result = QString("%1: %2").arg(paths::basicName(path), format::filesNumberAndSize(filesNumber, totalSize));
        }
        else {
            qDebug() << "Files::contentStatus | Canceled" << path;
        }
    }
    else if (fileInfo.isFile()) {
        result = format::fileNameAndSize(path);
    }
    else
        qDebug() << "Files::contentStatus | The 'path' doesn't exist";

    emit setStatusbarText(result);
    return result;
}

QString Files::contentStatus(const FileList &fileList)
{
    return format::filesNumberAndSize(fileList.size(), dataSize(fileList));
}

QString Files::itemInfo(const QAbstractItemModel* model, const QSet<FileStatus>& fileStatuses, const QModelIndex& rootIndex)
{
    int filesNumber = 0;
    qint64 dataSize = 0;
    TreeModelIterator it(model, rootIndex);

    while (it.hasNext()) {
        QVariant itData = it.nextFile().data(Column::ColumnStatus);

        if (itData.isValid()
            && (fileStatuses.isEmpty()
                || fileStatuses.contains(itData.value<FileStatus>()))) {

            dataSize += it.data(Column::ColumnSize).toLongLong();
            ++filesNumber;
        }
    }

    return format::filesNumberAndSize(filesNumber, dataSize);
}

void Files::folderContentsByType()
{
    if (initFolderPath.isEmpty()) {
        qDebug() << "Files::folderContentsByType | No initial data";
        return;
    }

    FileList fileList = getFileList(FilterRule(false));

    if (canceled)
        return;

    if (fileList.isEmpty()) {
        qDebug() << "Empty folder. No file types to display.";
        //emit folderContentsListCreated(initFolderPath, QList<ExtNumSize>());
        return;
    }

    QHash<QString, FileList> listsByType; // key = extension, value = list of files with that extension

    FileList::const_iterator filesIter;
    for (filesIter = fileList.constBegin(); filesIter != fileList.constEnd(); ++filesIter) {
        QString ext;

        if (tools::isDatabaseFile(filesIter.key()))
            ext = ExtNumSize::strVeretinoDb();
        else if (tools::isSummaryFile(filesIter.key()))
            ext = ExtNumSize::strShaFiles();
        else
            ext = QFileInfo(filesIter.key()).suffix().toLower();

        if (ext.isEmpty())
            ext = ExtNumSize::strNoType();

        listsByType[ext].insert(filesIter.key(), filesIter.value());
    }

    QList<ExtNumSize> combList;

    QHash<QString, FileList>::const_iterator iter;
    for (iter = listsByType.constBegin(); iter != listsByType.constEnd(); ++iter) {
        ExtNumSize t;
        t.extension = iter.key();
        t.filesNumber = iter.value().size();
        t.filesSize = dataSize(iter.value());
        combList.append(t);
    }

    emit folderContentsListCreated(initFolderPath, combList);
}

qint64 Files::dataSize()
{
    if (!initFolderPath.isEmpty())
        return dataSize(getFileList());
    else if (!initFileList.isEmpty())
        return dataSize(initFileList);
    else {
        qDebug() << "Files::dataSize() | No data to size return";
        return 0;
    }
}

qint64 Files::dataSize(const QString &folder)
{
    return dataSize(getFileList(folder));
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

QSet<FileStatus> Files::flagStatuses(const FileStatusFlag flag)
{
    switch (flag) {
    case FlagAvailable:
        return {NotChecked, Matched, Mismatched, Added, Updated};
    case FlagUpdatable:
        return {New, Missing, Mismatched};
    case FlagDbChanged:
        return {Added, Removed, Updated};
    case FlagChecked:
        return {Matched, Mismatched};
    case FlagMatched:
        return {Matched, Added, Updated};
    case FlagNewLost:
        return {New, Missing};
    default:
        return QSet<FileStatus>();
    }
}

void Files::cancelProcess()
{
    canceled = true;
}
