/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "files.h"
#include <QDirIterator>
#include <QDebug>
#include "tools.h"
#include "pathstr.h"
#include "treemodeliterator.h"
#include "treemodel.h"

Files::Files(QObject *parent)
    : QObject(parent)
{}

Files::Files(const QString &path, QObject *parent)
    : QObject(parent), m_fsPath(path)
{}

void Files::setProcState(const ProcState *procState)
{
    p_proc = procState;
}

void Files::setPath(const QString &path)
{
    m_fsPath = path;
}

bool Files::isCanceled() const
{
    return (p_proc && p_proc->isCanceled());
}

FileList Files::getFileList()
{
    return getFileList(m_fsPath);
}

FileList Files::getFileList(const FilterRule &filter)
{
    return getFileList(m_fsPath, filter);
}

FileList Files::getFileList(const QString &rootFolder, const FilterRule &filter)
{
    if (!QFileInfo(rootFolder).isDir()) {
        return FileList();
    }

    emit setStatusbarText(QStringLiteral(u"Creating a list of files..."));

    FileList resultList;
    QDirIterator it(rootFolder, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        const QString _fullPath = it.next();
        const QString _relPath = pathstr::relativePath(rootFolder, _fullPath);

        if (filter.isFileAllowed(_relPath)) {
            FileStatus _status = it.fileInfo().isReadable() ? FileStatus::NotSet
                                                            : FileStatus::UnPermitted;

            resultList.insert(_relPath, FileValues(_status, it.fileInfo().size()));
        }
    }

    if (isCanceled()) {
        qDebug() << "Files::getFileList | Canceled:" << rootFolder;
        emit setStatusbarText();
        return FileList();
    }

    emit setStatusbarText(format::filesNumber(resultList.size()) + QStringLiteral(u" found"));
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
    if (!QFileInfo(folderPath).isDir()) {
        qDebug() << "Files::isEmptyFolder | Wrong path:" << folderPath;
        return true;
    }

    QDirIterator it(folderPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (filter.isFileAllowed(it.next())) {
            return false;
        }
    }

    return true;
}

QString Files::firstDbFile(const QString &folderPath)
{
    QDirIterator it(folderPath, QDir::Files);
    while (it.hasNext()) {
        if (paths::isDbFile(it.next()))
            return it.filePath();
    }

    return QString();
}

QStringList Files::dbFiles(const QString &folderPath)
{
    QStringList _res;
    QDirIterator it(folderPath, QDir::Files);

    while (it.hasNext()) {
        if (paths::isDbFile(it.next()))
            _res << it.fileName();
    }

    return _res;
}

QString Files::getFolderSize()
{
    return getFolderSize(m_fsPath);
}

QString Files::getFolderSize(const QString &path)
{
    if (!QFileInfo(path).isDir())
        return QString();

    NumSize __n;

    // iterating
    QDirIterator it(path, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext() && !isCanceled()) {
        it.next();
        __n << it.fileInfo().size();
    }

    if (isCanceled()) {
        qDebug() << "Files::getFolderSize | Canceled" << path;
        return QString();
    }

    // result processing
    const QString _folderName = pathstr::basicName(path);
    const QString _folderSize = format::filesNumSize(__n);

    return tools::joinStrings(_folderName, _folderSize, Lit::s_sepColonSpace);
}

FileTypeList Files::getFileTypes(const QString &folderPath, FilterRule combine)
{
    emit setStatusbarText(QStringLiteral(u"Parsing folder contents..."));

    FileTypeList _res;
    QDirIterator it(folderPath, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        it.next();
        const qint64 _size = it.fileInfo().size();

        if (combine.hasAttribute(FilterAttribute::IgnoreUnpermitted)
            && !it.fileInfo().isReadable())
        {
            _res.m_combined[FilterAttribute::IgnoreUnpermitted] << _size;
        }
        else if (combine.hasAttribute(FilterAttribute::IgnoreSymlinks)
                 && it.fileInfo().isSymLink())
        {
            _res.m_combined[FilterAttribute::IgnoreSymlinks] << _size;
        }
        else {
            const QString _filename = it.fileName();

            if (paths::isDbFile(_filename))
                _res.m_combined[FilterAttribute::IgnoreDbFiles] << _size;
            else if (paths::isDigestFile(_filename))
                _res.m_combined[FilterAttribute::IgnoreDigestFiles] << _size;
            else
                _res.m_extensions[pathstr::suffix(_filename)] << _size;
        }
    }

    if (isCanceled()) {
        qDebug() << "Files::getFileTypes | Canceled:" << folderPath;
        emit setStatusbarText();
        return FileTypeList();
    }

    emit setStatusbarText(tools::joinStrings(_res.count(), QStringLiteral(u"types found")));
    return _res;
}

FileTypeList Files::getFileTypes(const QAbstractItemModel *model, const QModelIndex &rootIndex)
{
    FileTypeList _res;
    TreeModelIterator it(model, rootIndex);

    while (it.hasNext() && !isCanceled()) {
        it.nextFile();
        if (it.status() & FileStatus::CombAvailable) {
            const QString _file = it.path();

            if (paths::isDbFile(_file))
                _res.m_combined[FilterAttribute::IgnoreDbFiles] << it.size();
            else if (paths::isDigestFile(_file))
                _res.m_combined[FilterAttribute::IgnoreDigestFiles] << it.size();
            else
                _res.m_extensions[pathstr::suffix(_file)] << it.size();
        }
    }

    return _res;
}

NumSize Files::totalListed(const FileTypeList &typeList)
{
    NumSize res;

    if (!typeList.m_extensions.isEmpty()) {
        QHash<QString, NumSize>::const_iterator it;
        for (it = typeList.m_extensions.constBegin(); it != typeList.m_extensions.constEnd(); ++it)
            res.add(it.value());
    }

    if (!typeList.m_combined.isEmpty()) {
        QHash<FilterAttribute, NumSize>::const_iterator it2;
        for (it2 = typeList.m_combined.constBegin(); it2 != typeList.m_combined.constEnd(); ++it2)
            res.add(it2.value());
    }

    return res;
}

qint64 Files::dataSize()
{
    return !m_fsPath.isEmpty() ? dataSize(getFileList()) : 0;
}

qint64 Files::dataSize(const QString &folder)
{
    return dataSize(getFileList(folder));
}

qint64 Files::dataSize(const FileList &filelist)
{
    qint64 _total = 0;

    FileList::const_iterator iter;
    for (iter = filelist.constBegin(); iter != filelist.constEnd(); ++iter) {
        const qint64 _size = iter.value().size;
        if (_size > 0)
            _total += _size;
    }

    return _total;
}
