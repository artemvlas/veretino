/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "files.h"
#include <QDirIterator>
#include <QStandardPaths>
#include <QDebug>
#include "tools.h"
#include "treemodeliterator.h"
#include "treemodel.h"

const QString ExtNumSize::strNoType = "No type";
const QString ExtNumSize::strVeretinoDb = "Veretino DB";
const QString ExtNumSize::strShaFiles = "sha1/256/512";
const QString ExtNumSize::strNoPerm = "No Permissions";

const QString Files::desktopFolderPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

Files::Files(QObject *parent)
    : QObject(parent)
{}

Files::Files(const QString &path, QObject *parent)
    : QObject(parent), fsPath_(path)
{}

void Files::setProcState(const ProcState *procState)
{
    proc_ = procState;
}

void Files::setPath(const QString &path)
{
    fsPath_ = path;
}

bool Files::isCanceled() const
{
    return proc_ && proc_->isCanceled();
}

FileList Files::getFileList()
{
    return getFileList(fsPath_);
}

FileList Files::getFileList(const FilterRule &filter)
{
    return getFileList(fsPath_, filter);
}

FileList Files::getFileList(const QString &rootFolder, const FilterRule &filter)
{
    if (!QFileInfo(rootFolder).isDir()) {
        return FileList();
    }

    emit setStatusbarText("Creating a list of files...");

    FileList resultList; // result list

    QDir dir(rootFolder);
    QDirIterator it(rootFolder, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        const QString &_fullPath = it.next();
        const QString &_relPath = dir.relativeFilePath(_fullPath);

        if (filter.isFileAllowed(_relPath)) {
            QFileInfo fileInfo(_fullPath);
            FileStatus _status = fileInfo.isReadable() ? FileStatus::NotSet : FileStatus::Unreadable;

            resultList.insert(_relPath, FileValues(_status, fileInfo.size()));
        }
    }

    if (isCanceled()) {
        qDebug() << "Files::getFileList | Canceled:" << rootFolder;
        emit setStatusbarText();
        return FileList();
    }

    emit setStatusbarText(QString("%1 files found").arg(resultList.size()));
    return resultList;
}

FileList Files::getFileList(const QAbstractItemModel *model, const FileStatuses flag, const QModelIndex &rootIndex)
{
    if (!model)
        return FileList();

    FileList fileList;
    TreeModelIterator it(model, rootIndex);

    while (it.hasNext() && !isCanceled()) {
        it.nextFile();
        if (it.status() & flag) {
            fileList.insert(it.path(), FileValues(it.status(), it.size()));
        }
    }

    return fileList;
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

QString Files::getFolderSize()
{
    return getFolderSize(fsPath_);
}

QString Files::getFolderSize(const QString &path)
{   
    QFileInfo fileInfo(path);
    QString result;

    if (fileInfo.isDir()) {
        int filesNumber = 0;
        qint64 totalSize = 0;

        // iterating
        QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext() && !isCanceled()) {
            totalSize += QFileInfo(it.next()).size();
            ++filesNumber;
        }

        // result processing
        if (!isCanceled()) {
            const QString _folderName = paths::basicName(path);
            const QString _folderSize = (filesNumber > 0) ? format::filesNumberAndSize(filesNumber, totalSize) : "no files";

            result = QString("%1: %2")
                         .arg(_folderName, _folderSize);
        }
        else
            qDebug() << "Files::getFolderSize | Canceled" << path;
    }

    return result;
}

QString Files::itemInfo(const QAbstractItemModel* model, const FileStatuses flags, const QModelIndex &rootIndex)
{
    int filesNumber = 0;
    qint64 dataSize = 0;
    TreeModelIterator it(model, rootIndex);

    while (it.hasNext()) {
        const QVariant itData = it.nextFile().data(Column::ColumnStatus);

        if (itData.isValid()
            && (flags & itData.value<FileStatus>()))
        {
            dataSize += it.size();
            ++filesNumber;
        }
    }

    return format::filesNumberAndSize(filesNumber, dataSize);
}

QList<ExtNumSize> Files::getFileTypes()
{
    return getFileTypes(fsPath_);
}

QList<ExtNumSize> Files::getFileTypes(const QString &folderPath)
{
    return getFileTypes(getFileList(folderPath, FilterRule(false)));
}

QList<ExtNumSize> Files::getFileTypes(const QAbstractItemModel *model, const QModelIndex &rootIndex)
{
    return getFileTypes(getFileList(model, FileStatus::FlagAvailable, rootIndex));
}

QList<ExtNumSize> Files::getFileTypes(const FileList &fileList, bool excludeUnreadable)
{
    if (fileList.isEmpty())
        return QList<ExtNumSize>();

    QHash<QString, FileList> listsByType; // key = extension, value = list of files with that extension

    FileList::const_iterator filesIter;
    for (filesIter = fileList.constBegin(); filesIter != fileList.constEnd(); ++filesIter) {
        QString _ext;

        if (excludeUnreadable && (filesIter.value().status == FileStatus::Unreadable))
            _ext = ExtNumSize::strNoPerm;
        else if (tools::isDatabaseFile(filesIter.key()))
            _ext = ExtNumSize::strVeretinoDb;
        else if (tools::isSummaryFile(filesIter.key()))
            _ext = ExtNumSize::strShaFiles;
        else
            _ext = QFileInfo(filesIter.key()).suffix().toLower();

        if (_ext.isEmpty())
            _ext = ExtNumSize::strNoType;

        listsByType[_ext].insert(filesIter.key(), filesIter.value());
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

    return combList;
}

qint64 Files::dataSize()
{
    return !fsPath_.isEmpty() ? dataSize(getFileList()) : 0;
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
            const qint64 _size = iter.value().size;
            if (_size > 0)
                totalSize += _size;
        }
    }

    return totalSize;
}
