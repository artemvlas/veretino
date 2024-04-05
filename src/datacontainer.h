/*
 * This file is part of Veretino project.
 * GNU General Public License (GNU GPLv3).
 * https://github.com/artemvlas/veretino
*/
#ifndef DATACONTAINER_H
#define DATACONTAINER_H

#include <QObject>
#include <QCryptographicHash>
#include "treemodel.h"
#include "proxymodel.h"

class TreeModel;

struct MetaData {
    QCryptographicHash::Algorithm algorithm;
    QString workDir; // current working folder
    QString databaseFilePath;
    QString saveDateTime; // date and time the database was saved
    QString successfulCheckDateTime; // date and time of the last completely successful check (all files from the list are exists and match the checksums)
    FilterRule filter;
    bool isImported = false; // from db(json) file
    enum SaveResult {NotSaved, Saved, SavedToDesktop};
    SaveResult saveResult = NotSaved;
}; // struct MetaData

struct Numbers {
    int numChecksums = 0; // number of files with checksums
    qint64 totalSize = 0; // total size in bytes of all actual files for which there are checksums listed

    QHash<FileStatus, int> holder; // {enum FileStatus : int number of corresponding files}

    int numberOf(const FileStatus status) const
    {
        return holder.contains(status) ? holder.value(status) : 0;
    }

    int numberOf(const FileStatusFlag flag) const
    {
        if (flag == FileStatusFlag::FlagAvailable) // for proper display in a permanent status during the process
            return numChecksums - numberOf(FileStatus::Missing);

        return numberOf(Files::flagStatuses(flag));
    }

    int numberOf(const QSet<FileStatus> &statuses) const
    {
        int result = 0;

        QSet<FileStatus>::const_iterator it;
        for (it = statuses.constBegin(); it != statuses.constEnd(); ++it) {
            result += numberOf(*it);
        }

        return result;
    }

    bool contains(const FileStatus status) const
    {
        return numberOf(status) > 0;
    }

    bool contains(const FileStatusFlag flag) const
    {
        return numberOf(flag) > 0;
    }

    bool contains(const QSet<FileStatus> &statuses) const
    {
        return numberOf(statuses) > 0;
    }
}; // struct Numbers

class DataContainer : public QObject
{
    Q_OBJECT
public:
    explicit DataContainer(QObject *parent = nullptr);
    ~DataContainer();

    ProxyModel* setProxyModel();
    QString databaseFileName() const;
    QString backupFilePath() const;
    QString dbSubFolderAbsolutePath(const QModelIndex &subfolder) const; // returns the absolute path to the database subfolder
    QString branchDbFilePath(const QModelIndex &subfolder) const;
    bool isWorkDirRelative() const;
    bool isFilterApplied() const;
    bool contains(const FileStatus status, const QModelIndex &subfolder = QModelIndex()) const;
    bool contains(const FileStatusFlag flag, const QModelIndex &subfolder = QModelIndex()) const;
    bool contains(const QSet<FileStatus> &statuses, const QModelIndex &subfolder = QModelIndex()) const;
    bool isAllChecked() const;

    bool isBackupExists();
    bool makeBackup(bool forceOverwrite = false);
    bool restoreBackupFile();
    void removeBackupFile();
    void setSaveResult(const QString &dbFilePath);

    const Numbers& updateNumbers();
    Numbers getNumbers(const QModelIndex &rootIndex = QModelIndex()) const;
    static Numbers getNumbers(const QAbstractItemModel *model,
                              const QModelIndex &rootIndex = QModelIndex());

    // DATA
    TreeModel *model_ = new TreeModel(this);  // main data
    ProxyModel *proxyModel_ = new ProxyModel(model_, this);
    MetaData metaData;
    Numbers numbers;
}; // class DataContainer

#endif // DATACONTAINER_H
