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
#include "verdatetime.h"

class TreeModel;

struct MetaData {
    QCryptographicHash::Algorithm algorithm = QCryptographicHash::Sha256;
    FilterRule filter;
    QString workDir; // current working folder
    QString dbFilePath;

    VerDateTime datetime;

    enum DbFileState : quint8 { NoFile, Created, NotSaved, Saved };
    DbFileState dbFileState = NoFile;

    enum PropertyFlag : quint8 { NotSet = 0, FlagConst = 1 };
    quint8 flags = NotSet;
}; // struct MetaData

using DbFileState = MetaData::DbFileState;

class DataContainer : public QObject
{
    Q_OBJECT
public:
    explicit DataContainer(QObject *parent = nullptr);
    explicit DataContainer(const MetaData &meta, QObject *parent = nullptr);
    ~DataContainer();

    ProxyModel* setProxyModel();
    QString databaseFileName() const;
    QString backupFilePath() const;
    QString itemAbsolutePath(const QModelIndex &curIndex) const;        // returns the absolute path to the database item (file or subfolder)
    QString branch_path_existing(const QModelIndex &subfolder);         // returns the path to the found Branch, an empty string if not found; caches the result
    QString branch_path_composed(const QModelIndex &subfolder) const;   // returns the composed path regardless of the file's existence
    QString digestFilePath(const QModelIndex &fileIndex) const;
    QString basicDate() const;                                          // the date until which files are considered unmodified

    bool isDbFileState(DbFileState state) const;
    bool isWorkDirRelative() const;
    bool isFilterApplied() const;
    bool contains(const FileStatuses flags, const QModelIndex &subfolder = QModelIndex()) const;
    bool isAllChecked() const;
    bool isAllMatched(const QModelIndex &subfolder = QModelIndex()) const;
    bool isAllMatched(const Numbers &nums) const;
    bool isInCreation() const;
    bool isImmutable() const;           // has FlagConst
    bool hasPossiblyMovedItems() const; // has New and Missing

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
    MetaData metaData_;
    Numbers numbers_;

    QHash<QString, QModelIndex> _cacheMissing;
    QHash<QModelIndex, QString> _cacheBranches;
}; // class DataContainer

#endif // DATACONTAINER_H
