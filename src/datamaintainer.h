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

    // iterate the 'm_data->metaData.workDir' folder and add the finded files to the m_data->m_model
    int folderBasedData(FileStatus fileStatus = FileStatus::New);

    // returns 'true' if Added or Matched. returns false if Mismatched
    bool updateChecksum(const QModelIndex &fileRowIndex,
                        const QString &computedChecksum);

    bool importChecksum(const QModelIndex &file,
                        const QString &checksum);

    // changes the status of all items with <statuses> to the <newStatus>
    int changeStatuses(const FileStatuses statuses,
                       const FileStatus newStatus,
                       const QModelIndex &rootIndex = QModelIndex());

    // changes items status from <statuses> to FileStatus::Queued
    int addToQueue(const FileStatuses statuses,
                   const QModelIndex &rootIndex = QModelIndex());

    // clears stored checksum string, single file item
    void clearChecksum(const QModelIndex &fileIndex);

    // ... for all items with given statuses, returns done number
    int clearChecksums(const FileStatuses statuses,
                       const QModelIndex &rootIndex = QModelIndex());

    // clears the stored checksums of the Missing/Lost items, returns the done number
    int clearLostFiles();

    // move ReChecksum --> Checksum
    int updateMismatchedChecksums();

    // rolls back file statuses when canceling an operation
    void rollBackStoppedCalc(const QModelIndex &rootIndex, FileStatus prevStatus);

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

    // file: "filename (size)"; folder: contents (availability, size etc.)
    QString itemContentsInfo(const QModelIndex &curIndex);

    // variables
    DataContainer *m_data = nullptr; // main data

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

    // backup for the duration of m_data setup, should be deleted after setting the m_data to View
    DataContainer *m_oldData = nullptr;
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
