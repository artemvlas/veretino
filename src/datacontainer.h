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
    void addFile(const FileStatus status, const qint64 size)
    {
        amounts_[status]++;
        sizes_[status] += size;
    }

    qint64 totalSize(const FileStatuses flag) const
    {
        qint64 result = 0;
        QHash<FileStatus, qint64>::const_iterator it;

        for (it = sizes_.constBegin(); it != sizes_.constEnd(); ++it) {
            if (it.key() & flag) {
                result += it.value();
            }
        }

        return result;
    }

    int numberOf(const FileStatuses flag) const
    {
        int result = 0;
        QHash<FileStatus, int>::const_iterator it;

        for (it = amounts_.constBegin(); it != amounts_.constEnd(); ++it) {
            if (it.key() & flag)
                result += it.value();
        }

        return result;
    }

    bool contains(const FileStatuses flags) const
    {
        return numberOf(flags) > 0;
    }

private:
    QHash<FileStatus, int> amounts_; // {FileStatus : number of corresponding files}
    QHash<FileStatus, qint64> sizes_; // {FileStatus : total size}
}; // struct Numbers

class DataContainer : public QObject
{
    Q_OBJECT
public:
    explicit DataContainer(QObject *parent = nullptr);
    explicit DataContainer(const MetaData &metadata, QObject *parent = nullptr);
    ~DataContainer();

    ProxyModel* setProxyModel();
    QString databaseFileName() const;
    QString backupFilePath() const;
    QString itemAbsolutePath(const QModelIndex &curIndex) const; // returns the absolute path to the database item (file or subfolder)
    QString getBranchFilePath(const QModelIndex &subfolder, bool existing = false) const;
    // ^ existing = false: returns the predefined path to the branch database file, regardless of the file's existence
    // true: returns the path to the existing branch's database file; empty str if missing

    bool isWorkDirRelative() const;
    bool isFilterApplied() const;
    bool contains(const FileStatuses flags, const QModelIndex &subfolder = QModelIndex()) const;
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
