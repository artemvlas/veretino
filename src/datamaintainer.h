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
    bool setItemValue(const QModelIndex &fileIndex,
                      Column column,
                      const QVariant &value = QVariant());
    void setFileStatus(const QModelIndex &index, FileStatus status);
    void setConsiderDateModified(bool consider);
    void updateDateTime();
    void updateVerifDateTime();
    void updateNumbers();
    void updateNumbers(const QModelIndex &fileIndex,
                       const FileStatus statusBefore);
    void updateNumbers(const FileStatus status_old,
                       const FileStatus status_new,
                       const qint64 size = 0);
    void moveNumbers(const FileStatus before, const FileStatus after);
    void setDbFileState(DbFileState state);

    int folderBasedData(FileStatus fileStatus = FileStatus::New);                         // iterate the 'm_data->metaData.workDir' folder and add the finded files to the m_data->m_model

    bool updateChecksum(const QModelIndex &fileRowIndex,                                  // returns 'true' if Added or Matched. returns false if Mismatched
                        const QString &computedChecksum);

    bool importChecksum(const QModelIndex &file,
                        const QString &checksum);

    int changeStatuses(const FileStatuses statuses,                                       // changes the status of all items with <statuses> to the <newStatus>
                       const FileStatus newStatus,
                       const QModelIndex &rootIndex = QModelIndex());

    int addToQueue(const FileStatuses statuses,                                           // changes items status from <statuses> to FileStatus::Queued
                   const QModelIndex &rootIndex = QModelIndex());

    void clearChecksum(const QModelIndex &fileIndex);                                     // clears stored checksum string, single file item
    int clearChecksums(const FileStatuses statuses,                                       // ... for all items with given statuses, returns done number
                       const QModelIndex &rootIndex = QModelIndex());

    int clearLostFiles();                                                                 // clears the stored checksums of the Missing/Lost items, returns the done number
    int updateMismatchedChecksums();                                                      // move ReChecksum --> Checksum
    void rollBackStoppedCalc(const QModelIndex &rootIndex, FileStatus prevStatus);        // rolls back file statuses when canceling an operation

    bool itemFileRemoveLost(const QModelIndex &fileIndex);
    bool itemFileUpdateChecksum(const QModelIndex &fileIndex);
    bool tryMoved(const QModelIndex &file, const QString &checksum);

    TreeModel* createDataModel(const VerJson &json, const MetaData &meta);
    bool importJson(const QString &filePath);
    bool exportToJson();
    bool saveJsonFile(VerJson *json);
    void forkJsonDb(const QModelIndex &rootFolder);
    int importBranch(const QModelIndex &rootFolder);

    bool isDataNotSaved() const;

    QString itemContentsInfo(const QModelIndex &curIndex);                                 // file: "filename (size)"; folder: contents (availability, size etc.)

    // variables
    DataContainer *m_data = nullptr;                                                       // main data

public slots:
    void clearData();
    void clearOldData();
    void saveData();

private:
    void connections();
    bool isCanceled() const;
    MetaData getMetaData(const VerJson &json) const;
    FileValues makeFileValues(const QString &filePath, const QString &basicDate) const;
    VerJson* makeJson(const QModelIndex &rootFolder = QModelIndex());
    bool isPresentInWorkDir(const VerJson &json, const QString &workDir) const;
    QString findWorkDir(const VerJson &json) const;

    DataContainer *m_oldData = nullptr; // backup for the duration of m_data setup, should be deleted after setting the m_data to View
    const ProcState *m_proc = nullptr;

    bool m_considerFileModDate;

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
