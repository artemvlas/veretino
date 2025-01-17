/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DATAMAINTAINER_H
#define DATAMAINTAINER_H

#include <QObject>
#include "datacontainer.h"
#include "verjson.h"
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
    void setFileStatus(const QModelIndex &_index, FileStatus _status);
    void setConsiderDateModified(bool consider);
    void updateDateTime();
    void updateVerifDateTime();
    void updateNumbers();
    void updateNumbers(const QModelIndex &fileIndex, const FileStatus statusBefore);
    void updateNumbers(const FileStatus status_old, const FileStatus status_new, const qint64 size = 0);
    void moveNumbers(const FileStatus _before, const FileStatus _after);
    void setDbFileState(DbFileState state);

    // iterate the 'data_->metaData.workDir' folder and add the finded files to the data_->model_
    int folderBasedData(FileStatus fileStatus = FileStatus::New);

    // returns 'true' if Added or Matched. returns false if Mismatched
    bool updateChecksum(const QModelIndex &fileRowIndex,
                        const QString &computedChecksum);

    bool importChecksum(const QModelIndex &_file,
                        const QString &_checksum);

    int changeFilesStatus(const FileStatuses flags,
                          const FileStatus newStatus,
                          const QModelIndex &rootIndex = QModelIndex());

    // changes statuses of files in data_->model_ from <flags> to FileStatus::Queued
    int addToQueue(const FileStatuses flags,
                   const QModelIndex &rootIndex = QModelIndex());

    void clearChecksum(const QModelIndex &fileIndex);
    int clearChecksums(const FileStatuses flags,
                       const QModelIndex &rootIndex = QModelIndex());

    int clearLostFiles();                                                                  // returns the number of cleared
    int updateMismatchedChecksums();                                                       // returns the number of updated checksums
    void rollBackStoppedCalc(const QModelIndex &rootIndex, FileStatus prevStatus);         // rolls back file statuses when canceling an operation

    bool itemFileRemoveLost(const QModelIndex &fileIndex);
    bool itemFileUpdateChecksum(const QModelIndex &fileIndex);
    bool tryMoved(const QModelIndex &_file, const QString &_checksum);

    bool importJson(const QString &filePath);
    bool exportToJson();
    bool saveJsonFile(VerJson *_json);
    void forkJsonDb(const QModelIndex &rootFolder);
    int importBranch(const QModelIndex &rootFolder);

    bool isDataNotSaved() const;

    // if file - "filename (size)", if folder - folder contents (availability, size etc.)
    QString itemContentsInfo(const QModelIndex &curIndex);

    // variables
    DataContainer *data_ = nullptr;     // main data

public slots:
    void clearData();
    void clearOldData();
    void saveData();

private:
    void connections();
    bool isCanceled() const;
    MetaData getMetaData(const VerJson &_json) const;
    FileValues makeFileValues(const QString &filePath, const QString &basicDate) const;
    VerJson* makeJson(const QModelIndex &rootFolder = QModelIndex());
    QString findWorkDir(const VerJson &_json) const;
    bool isPresentInWorkDir(const QString &workDir, const QJsonObject &fileList) const;

    DataContainer *oldData_ = nullptr; // backup for the duration of data_ setup, should be deleted after setting the data_ to View
    const ProcState *proc_ = nullptr;

    bool considerFileModDate;

signals:
    void databaseUpdated();
    void setStatusbarText(const QString &text = QString()); // text to statusbar
    void numbersUpdated();
    void showMessage(const QString &text, const QString &title = "Info");
    void subDbForked(const QString &forkedDbFilePath);
    void dbFileStateChanged(DbFileState state);
    void failedDataCreation();
    void failedJsonSave(VerJson *p_unsaved);
}; // class DataMaintainer

#endif // DATAMAINTAINER_H
