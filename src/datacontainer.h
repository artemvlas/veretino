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
    QString workDir;      // current working folder
    QString dbFilePath;   // path to the db file
    QString comment;      // custom comment string/text

    FilterRule filter;    // file filtering rules for the current database
    VerDateTime datetime; // time stamps: date and time of creation, update, verification

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
    explicit DataContainer(const MetaData &meta, TreeModel *data, QObject *parent = nullptr);
    ~DataContainer();

    // replace the current data and metadata with new ones, clears numbers and cache
    void set(const MetaData &meta, TreeModel *data);

    // set empty data and metadata
    void clear();

    // is m_model empty
    bool isEmpty() const;

    /*** DATA ***/
    // main data model
    TreeModel *m_model = new TreeModel(this);

    // view proxy (sort, filter)
    ProxyModel *m_proxy = new ProxyModel(m_model, this);

    MetaData m_metadata;
    Numbers m_numbers;

    QHash<QString, QModelIndex> m_cacheMissing;
    QHash<QModelIndex, QString> m_cacheBranches;
}; // class DataContainer

/*** <!!!> ***/
/*** DataHelper is a TEMPORARY holder of functions separated from the DataContainer ***/
/*** They will be moved or changed in the future ***/
struct DataHelper {
    static QString databaseFileName(const DataContainer *data);
    static QString backupFilePath(const DataContainer *data);
    static QString digestFilePath(const DataContainer *data, const QModelIndex &fileIndex);

    // returns the absolute path (workdir + path in db) to the db item (file or subfolder)
    static QString itemAbsolutePath(const DataContainer *data, const QModelIndex &curIndex);

    // returns the path to the found Branch, an empty string if not found; caches the result
    static QString branch_path_existing(DataContainer *data, const QModelIndex &subfolder);

    // returns the composed path regardless of the file's existence
    static QString branch_path_composed(const DataContainer *data, const QModelIndex &subfolder);

    static bool isDbFileState(const DataContainer *data, DbFileState state);

    static bool isWorkDirRelative(const DataContainer *data);

    static bool isFilterApplied(const DataContainer *data);

    static bool contains(const DataContainer *data, const FileStatuses flags,
                         const QModelIndex &subfolder = QModelIndex());

    static bool isAllChecked(const DataContainer *data);

    static bool isAllMatched(const DataContainer *data, const QModelIndex &subfolder = QModelIndex());
    static bool isAllMatched(const Numbers &nums);
    static bool isInCreation(const DataContainer *data);

    // has FlagConst
    static bool isImmutable(const DataContainer *data);

    // has New and Missing
    static bool hasPossiblyMovedItems(const DataContainer *data);

    static const Numbers& updateNumbers(DataContainer *data);

    static Numbers getNumbers(const DataContainer *data, const QModelIndex &rootIndex = QModelIndex());

    static Numbers getNumbers(const QAbstractItemModel *model,
                              const QModelIndex &rootIndex = QModelIndex());


    static bool isBackupExists(const DataContainer *data);
    static bool makeBackup(const DataContainer *data, bool forceOverwrite = false);
    static bool restoreBackupFile(const DataContainer *data);
    static void removeBackupFile(const DataContainer *data);

}; // struct DataHelper

#endif // DATACONTAINER_H
