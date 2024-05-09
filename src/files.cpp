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

const QString ExtNumSize::strNoType = "No type";
const QString ExtNumSize::strVeretinoDb = "Veretino DB";
const QString ExtNumSize::strShaFiles = "Sha-files";

Files::Files(QObject *parent)
    : QObject(parent)
{}

Files::Files(const QString &initPath, QObject *parent)
    : QObject(parent), initPath_(initPath)
{}

FileList Files::getFileList()
{
    return getFileList(initPath_);
}

FileList Files::getFileList(const FilterRule &filter)
{
    return getFileList(initPath_, filter);
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

void Files::contentStatus()
{
    contentStatus(initPath_);
}

void Files::contentStatus(const QString &path)
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

        if (!canceled)
            result = QString("%1: %2").arg(paths::basicName(path), format::filesNumberAndSize(filesNumber, totalSize));
        else
            qDebug() << "Files::contentStatus | Canceled" << path;
    }
    else if (fileInfo.isFile())
        result = format::fileNameAndSize(path);
    else
        qDebug() << "Files::contentStatus | The 'path' doesn't exist";

    if (!result.isEmpty())
        emit setStatusbarText(result);

    emit finished();
}

QString Files::itemInfo(const QAbstractItemModel* model, const FileStatuses flags, const QModelIndex &rootIndex)
{
    int filesNumber = 0;
    qint64 dataSize = 0;
    TreeModelIterator it(model, rootIndex);

    while (it.hasNext()) {
        QVariant itData = it.nextFile().data(Column::ColumnStatus);

        if (itData.isValid()
            && (flags & itData.value<FileStatus>())) {

            dataSize += it.size();
            ++filesNumber;
        }
    }

    return format::filesNumberAndSize(filesNumber, dataSize);
}

void Files::folderContentsByType()
{
    _folderContentsByType();
    emit finished();
}

void Files::_folderContentsByType()
{
    if (initPath_.isEmpty()) {
        qDebug() << "Files::folderContentsByType | No initial data";
        return;
    }

    FileList fileList = getFileList(FilterRule(false));

    if (canceled)
        return;

    if (fileList.isEmpty()) {
        qDebug() << "Empty folder. No file types to display.";
        return;
    }

    QHash<QString, FileList> listsByType; // key = extension, value = list of files with that extension

    FileList::const_iterator filesIter;
    for (filesIter = fileList.constBegin(); filesIter != fileList.constEnd(); ++filesIter) {
        QString ext;

        if (tools::isDatabaseFile(filesIter.key()))
            ext = ExtNumSize::strVeretinoDb;
        else if (tools::isSummaryFile(filesIter.key()))
            ext = ExtNumSize::strShaFiles;
        else
            ext = QFileInfo(filesIter.key()).suffix().toLower();

        if (ext.isEmpty())
            ext = ExtNumSize::strNoType;

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

    emit folderContentsListCreated(initPath_, combList);
}

qint64 Files::dataSize()
{
    return !initPath_.isEmpty() ? dataSize(getFileList()) : 0;
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

void Files::cancelProcess()
{
    canceled = true;
}
