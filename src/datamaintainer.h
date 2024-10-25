/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DATAMAINTAINER_H
#define DATAMAINTAINER_H

#include <QObject>
#include "datacontainer.h"
#include "jsondb.h"
#include "procstate.h"

class DataMaintainer : public QObject
{
    Q_OBJECT
public:
    explicit DataMaintainer(QObject *parent = nullptr);
    explicit DataMaintainer(DataContainer *initData, QObject *parent = nullptr);
    ~DataMaintainer();

    // functions() --->>
    void setProcState(const ProcState *procState);
    void setSourceData();
    void setSourceData(const MetaData &meta);
    bool setSourceData(DataContainer *sourceData);
    bool setItemValue(const QModelIndex &fileIndex, Column column, const QVariant &value = QVariant());
    void setConsiderDateModified(bool consider);
    void updateDateTime();
    void updateVerifDateTime();
    void updateNumbers();
    void updateNumbers(const QModelIndex &fileIndex, const FileStatus statusBefore);
    void updateNumbers(const FileStatus status_old, const FileStatus status_new, const qint64 size = 0);
    void moveNumbers(const FileStatus _before, const FileStatus _after);
    void setDbFileState(DbFileState state);

    // iterate the 'data_->metaData.workDir' and add the finded files to the data_->model_
    int folderBasedData(FileStatus fileStatus = FileStatus::New,
                       bool ignoreUnreadable = true);

    // returns 'true' if Added or Matched. returns false if Mismatched
    bool updateChecksum(const QModelIndex &fileRowIndex,
                        const QString &computedChecksum);

    int changeFilesStatus(const FileStatuses flags,
                          const FileStatus newStatus,
                          const QModelIndex &rootIndex = QModelIndex());

    // changes statuses of files in data_->model_ from <flags> to FileStatus::Queued
    int addToQueue(const FileStatuses flags,
                   const QModelIndex &rootIndex = QModelIndex());

    int clearChecksums(const FileStatuses flags,
                       const QModelIndex &rootIndex = QModelIndex());

    int clearLostFiles(); // returns the number of cleared
    int updateMismatchedChecksums(); // returns the number of updated checksums
    void rollBackStoppedCalc(const QModelIndex &rootIndex, FileStatus prevStatus);

    bool itemFileRemoveLost(const QModelIndex &fileIndex);
    bool itemFileUpdateChecksum(const QModelIndex &fileIndex);

    bool importJson(const QString &jsonFilePath);
    void exportToJson();
    void forkJsonDb(const QModelIndex &rootFolder);

    bool isDataNotSaved() const;

    // if file - "filename (size)", if folder - folder contents (availability, size etc.)
    QString itemContentsInfo(const QModelIndex &curIndex);

    // variables
    DataContainer *data_ = nullptr; // main data
    JsonDb *json_ = new JsonDb(this);

public slots:
    void clearData();
    void clearOldData();
    void saveData();

private:
    void connections();
    bool isCanceled() const;

    DataContainer *oldData_ = nullptr; // backup for the duration of data_ setup, should be deleted after setting the data_ to View
    const ProcState *proc_ = nullptr;

signals:
    void databaseUpdated();
    void setStatusbarText(const QString &text = QString()); // text to statusbar
    void numbersUpdated();
    void showMessage(const QString &text, const QString &title = "Info");
    void subDbForked(const QString &forkedDbFilePath);
    void dbFileStateChanged(bool isNotSaved);
    void failedDataCreation();
}; // class DataMaintainer

#endif // DATAMAINTAINER_H
