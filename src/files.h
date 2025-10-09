/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef FILES_H
#define FILES_H

#include <QObject>
#include <QAbstractItemModel>
#include "filterrule.h"
#include "procstate.h"
#include "filevalues.h"

struct FileTypeList {
    // {file extension (suffix) : files number and size}
    QHash<QString, NumSize> extensions;

    // {combined type like 'SymLinks' or 'UnPermitted' : files number and size}
    QHash<FilterAttribute, NumSize> combined;

    int count() const { return extensions.size() + combined.size(); }
    bool isEmpty() const { return count() == 0; }
    explicit operator bool() const { return !isEmpty(); }
}; // struct FileTypeList

class Files : public QObject
{
    Q_OBJECT

public:
    explicit Files(QObject *parent = nullptr);
    explicit Files(const QString &path, QObject *parent = nullptr);

    // functions
    void setProcState(const ProcState *procState);

    // path to file or folder >> 'fsPath_'
    void setPath(const QString &path);

    // 'fsPath_' --> getFileList(const QString &rootFolder)
    FileList getFileList();

    // return filtered filelist: can ignore or include only files with specified extensions
    FileList getFileList(const FilterRule &filter);

    FileList getFileList(const QString &rootFolder,
                         const FilterRule &filter = FilterRule());

    FileList getFileList(const QAbstractItemModel *model,
                         const FileValues::FileStatuses flag,
                         const QModelIndex &rootIndex = QModelIndex());

    // total size of all files in the 'fsPath_' folder
    qint64 dataSize();

    // total size of getFileList('folder')
    qint64 dataSize(const QString &folder);

    // total size of all files in the 'filelist'
    static qint64 dataSize(const FileList &filelist);

    // returns "folder name: number of files (redable size)"
    QString getFolderSize();
    QString getFolderSize(const QString &path);

    // returns a list of file types (extensions) with files number and their size
    // FilterAttributes are used to combine types
    FileTypeList getFileTypes(const QString &folderPath, FilterRule combine);
    FileTypeList getFileTypes(const QAbstractItemModel *model,
                              const QModelIndex &rootIndex = QModelIndex());

    static NumSize totalListed(const FileTypeList &_typeList);

    // checks whether there are any (or filtered) files the folder/subfolders
    static bool isEmptyFolder(const QString &folderPath,
                              const FilterRule &filter = FilterRule(FilterAttribute::NoAttributes));
    static QString firstDbFile(const QString &folderPath); // returns full path
    static QStringList dbFiles(const QString &folderPath); // file names only

private:
    bool isCanceled() const;

    // variables
    QString m_fsPath; // path to the File or Folder specified when creating the object
    const ProcState *m_proc = nullptr;

signals:
    void setStatusbarText(const QString &text = QString());
}; // class Files

#endif // FILES_H
