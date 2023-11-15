/* This file is part of the Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
 * These classes are used to store, organize, manage the data.
 * For each database, a separate object is created that stores checksums, all lists of files for various needs,
 * info about the algorithm type, relevance, etc. Objects can perform basic tasks of sorting, filtering, comparing data.
*/
#ifndef DATACONTAINER_H
#define DATACONTAINER_H

#include <QObject>
#include <QCryptographicHash>
#include "files.h"
#include "treemodel.h"
#include "proxymodel.h"
#include "tools.h"

class TreeModel;

struct MetaData {
    QCryptographicHash::Algorithm algorithm;
    QString workDir; // current working folder
    QString databaseFilePath;
    QString saveDateTime; // date and time the database was saved
    QString about; // contains a brief description of the item changes or status, if any
    FilterRule filter;
};

struct Numbers {
    int numChecksums = 0; // number of files with checksums
    int numImported = 0; // number of imported from database files
    int numMatched = 0; // number of check files with matched checksums
    int numMismatched = 0; // ... mismatched checksums
    int numAvailable = 0; // the number of files that exist on the disk and are readable, for which checksums are stored
    int numNewFiles = 0;
    int numMissingFiles = 0;
    int numUnreadable = 0;
    int numNotChecked = 0;
    int numQueued = 0;
    int numChecksumUpdated = 0;
    int numAdded = 0;

    qint64 totalSize = 0; // total size in bytes of all files for which there are checksums listed
};

class DataContainer : public QObject
{
    Q_OBJECT
public:
    explicit DataContainer(QObject *parent = nullptr) : QObject(parent){}

    MetaData metaData;
    Numbers numbers;

    TreeModel *model_ = new TreeModel(this);  // main data
    ProxyModel *proxyModel_ = new ProxyModel(model_, this);

    ProxyModel* setProxyModel()
    {
        if (proxyModel_)
            delete proxyModel_;

        proxyModel_ = new ProxyModel(model_, this);
        return proxyModel_;
    }

    QString databaseFileName() const
    {
        return paths::basicName(metaData.databaseFilePath);
    }

    bool isWorkDirRelative() const
    {
        return (paths::parentFolder(metaData.databaseFilePath) == metaData.workDir);
    }

    bool isFilterApplied() const
    {
        return !metaData.filter.extensionsList.isEmpty();
    }

    //DataContainer(){}
    //DataContainer(const MetaData &metadata) : metaData(metadata){}
};

class DataMaintainer : public QObject
{
    Q_OBJECT
public:
    explicit DataMaintainer(QObject *parent = nullptr);
    explicit DataMaintainer(DataContainer *initData, QObject *parent = nullptr);
    ~DataMaintainer();

    // functions() --->>
    void setSourceData();
    void setSourceData(DataContainer *sourceData);
    void clearData();

    // iterate the 'data_->metaData.workDir' and add the finded files to the data_->model_
    int addActualFiles(FileStatus addedFileStatus = Files::New, bool ignoreUnreadable = true);

    void updateNumbers();
    Numbers updateNumbers(const QAbstractItemModel* model,
                          const QModelIndex& rootIndex = QModelIndex());

    qint64 totalSizeOfListedFiles(FileStatus fileStatus, const QModelIndex& rootIndex = QModelIndex());
    qint64 totalSizeOfListedFiles(const QSet<FileStatus>& fileStatuses = QSet<FileStatus>(),
                                  const QModelIndex& rootIndex = QModelIndex());
    static qint64 totalSizeOfListedFiles(const QAbstractItemModel* model,
                                         const QSet<FileStatus>& fileStatuses = QSet<FileStatus>(),
                                         const QModelIndex& rootIndex = QModelIndex());

    bool updateChecksum(QModelIndex fileRowIndex,
                        const QString &computedChecksum); // returns 'true' if Added or Matched. returns false if Mismatched

    int changeFilesStatuses(FileStatus currentStatus,
                            FileStatus newStatus,
                            const QModelIndex &rootIndex = QModelIndex());

    QString getStoredChecksum(const QModelIndex &fileRowIndex);

    int clearDataFromLostFiles(); // returns the number of cleared
    int updateMismatchedChecksums(); // returns the number of updated checksums

    void importJson(const QString &jsonFilePath);
    void exportToJson();

    QString itemContentsInfo(const QModelIndex &curIndex); // if file - "filename (size)", if folder - folder contents (availability, size etc.)

    void dbStatus(); // info about current DB

    bool makeBackup(bool forceOverwrite = false);
    void removeBackupFile();

    QModelIndex sourceIndex(const QModelIndex &curIndex); // checks whether the curIndex belongs to the data_->proxyModel, if so, returns the mapToSource

    // variables
    DataContainer *data_ = nullptr;
    bool canceled = false;

public slots:
    void cancelProcess();

signals:
    void setStatusbarText(const QString &text = QString()); // text to statusbar
    void setPermanentStatus(const QString &text = QString());
    void showMessage(const QString &text, const QString &title = "Info");
    void sendProxyModel(ProxyModel* proxyModel);
};

#endif // DATACONTAINER_H
