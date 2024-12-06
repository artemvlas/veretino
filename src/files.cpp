/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "files.h"
#include <QDirIterator>
#include <QDebug>
#include "tools.h"
#include "treemodeliterator.h"
#include "treemodel.h"

const QString Files::strNoType = QStringLiteral(u"No type");
const QString Files::strVeretinoDb = QStringLiteral(u"Veretino DB");
const QString Files::strShaFiles = QStringLiteral(u"sha1/256/512");
const QString Files::strNoPerm = QStringLiteral(u"No Permissions");

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
    return (proc_ && proc_->isCanceled());
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

    emit setStatusbarText(QStringLiteral(u"Creating a list of files..."));

    FileList resultList;
    QDirIterator it(rootFolder, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        const QString _fullPath = it.next();
        const QString _relPath = paths::relativePath(rootFolder, _fullPath);

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
    return getFolderSize(fsPath_);
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
    const QString _folderName = paths::basicName(path);
    const QString _folderSize = format::filesNumSize(__n);

    return tools::joinStrings(_folderName, _folderSize, Lit::s_sepColonSpace);
}

FileTypeList Files::getFileTypes(const QString &folderPath, bool excludeUnPerm)
{
    emit setStatusbarText(QStringLiteral(u"Parsing folder contents..."));

    FileTypeList _res;
    QDirIterator it(folderPath, QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext() && !isCanceled()) {
        it.next();
        QString _ext = (excludeUnPerm && !it.fileInfo().isReadable()) ? strNoPerm
                                                                      : suffixName(it.fileName());
        _res[_ext] << it.fileInfo().size();
    }

    if (isCanceled()) {
        qDebug() << "Files::getFileTypes | Canceled:" << folderPath;
        emit setStatusbarText();
        return FileTypeList();
    }

    emit setStatusbarText(tools::joinStrings(_res.size(), QStringLiteral(u"types found")));
    return _res;
}

FileTypeList Files::getFileTypes(const QAbstractItemModel *model, const QModelIndex &rootIndex)
{
    FileTypeList _res;
    TreeModelIterator it(model, rootIndex);

    while (it.hasNext() && !isCanceled()) {
        it.nextFile();
        if (it.status() & FileStatus::CombAvailable) {
            const QString _ext = suffixName(it.path());
            _res[_ext] << it.size();
        }
    }

    return _res;
}

NumSize Files::totalListed(const FileTypeList &_typeList)
{
    NumSize _res;
    FileTypeList::const_iterator it;
    for (it = _typeList.constBegin(); it != _typeList.constEnd(); ++it) {
        _res.add(it.value());
    }

    return _res;
}

QString Files::suffixName(const QString &_file)
{
    QString _ext;

    if (paths::isDbFile(_file))
        _ext = strVeretinoDb;
    else if (paths::isDigestFile(_file))
        _ext = strShaFiles;
    else
        _ext = paths::suffix(_file);

    if (_ext.isEmpty())
        _ext = strNoType;

    return _ext;
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
    qint64 _total = 0;

    FileList::const_iterator iter;
    for (iter = filelist.constBegin(); iter != filelist.constEnd(); ++iter) {
        const qint64 _size = iter.value().size;
        if (_size > 0)
            _total += _size;
    }

    return _total;
}
