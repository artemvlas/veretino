/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DATACONTAINER_H
#define DATACONTAINER_H

#include <QObject>
#include <QCryptographicHash>
#include "treemodel.h"
#include "proxymodel.h"
#include "numbers.h"

class TreeModel;

struct MetaData {
    QCryptographicHash::Algorithm algorithm = QCryptographicHash::Sha256;
    FilterRule filter;
    QString workDir; // current working folder
    QString databaseFilePath;

    // DateVerified == (all files exist and match the checksums)
    enum DateTimeStr { DateCreated, DateUpdated, DateVerified };
    QString datetime[3];

    enum DbFileState { NoFile, Created, NotSaved, Saved };
    DbFileState dbFileState = NoFile;

    enum DbFlag { NotSet, FlagConst };
    DbFlag flags = NotSet;
}; // struct MetaData

using DbFileState = MetaData::DbFileState;
using DateTimeStr = MetaData::DateTimeStr;

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

    bool isDbFileState(DbFileState state) const;
    bool isWorkDirRelative() const;
    bool isFilterApplied() const;
    bool contains(const FileStatuses flags, const QModelIndex &subfolder = QModelIndex()) const;
    bool isAllChecked() const;
    bool isAllMatched() const;
    bool isInCreation() const;
    bool isImmutable() const; // has FlagConst

    bool isBackupExists() const;
    bool makeBackup(bool forceOverwrite = false) const;
    bool restoreBackupFile() const;
    void removeBackupFile() const;

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
