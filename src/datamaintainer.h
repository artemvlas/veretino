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
    bool setSourceData(DataContainer *sourceData);
    void updateDateTime();
    void updateVerifDateTime();
    void updateNumbers();
    void updateNumbers(const QModelIndex &fileIndex, const FileStatus statusBefore);
    void setDbFileState(DbFileState state);

    // iterate the 'data_->metaData.workDir' and add the finded files to the data_->model_
    int addActualFiles(FileStatus fileStatus = FileStatus::New,
                       bool ignoreUnreadable = true);

    qint64 totalSizeOfListedFiles(const FileStatuses flags,
                                  const QModelIndex &rootIndex = QModelIndex());

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
    DataContainer *oldData_ = nullptr; // backup for the duration of data_ setup, deleted by a signal after setting the data_ to View
    const ProcState *proc_ = nullptr;

signals:
    void databaseUpdated();
    void setStatusbarText(const QString &text = QString()); // text to statusbar
    void numbersUpdated();
    void showMessage(const QString &text, const QString &title = "Info");
    void subDbForked(const QString &forkedDbFilePath);
    void dbFileStateChanged(bool isNotSaved);
}; // class DataMaintainer

#endif // DATAMAINTAINER_H
