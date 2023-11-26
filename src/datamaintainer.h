// This file is part of Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
#ifndef DATAMAINTAINER_H
#define DATAMAINTAINER_H

#include <QObject>
#include "datacontainer.h"
#include "jsondb.h"

class DataMaintainer : public QObject
{
    Q_OBJECT
public:
    explicit DataMaintainer(QObject *parent = nullptr);
    explicit DataMaintainer(DataContainer *initData, QObject *parent = nullptr);
    ~DataMaintainer();

    // functions() --->>
    void setSourceData();
    bool setSourceData(DataContainer *sourceData);
    void clearData();
    void clearOldData();

    // iterate the 'data_->metaData.workDir' and add the finded files to the data_->model_
    int addActualFiles(FileStatus addedFileStatus = FileStatus::New,
                       bool ignoreUnreadable = true,
                       bool finalProcess = false); // <finalProcess> -->> whether it sends a process end signal or not

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

    int addToQueue(FileStatus currentStatus,
                   const QModelIndex &rootIndex = QModelIndex()); // changes statuses of files in data_->model_ from <currentStatus> to FileStatus::Queued

    QString getStoredChecksum(const QModelIndex &fileRowIndex);

    int clearDataFromLostFiles(bool finalProcess = false); // returns the number of cleared
    int updateMismatchedChecksums(bool finalProcess = false); // returns the number of updated checksums

    bool importJson(const QString &jsonFilePath);
    void exportToJson(bool finalProcess = true);

    QString itemContentsInfo(const QModelIndex &curIndex); // if file - "filename (size)", if folder - folder contents (availability, size etc.)

    QModelIndex sourceIndex(const QModelIndex &curIndex); // checks whether the curIndex belongs to the data_->proxyModel, if so, returns the mapToSource

    // variables
    DataContainer *data_ = nullptr; // main data
    JsonDb *json = new JsonDb;
    bool canceled = false;

private:
    void connections();
    DataContainer *oldData_ = nullptr; // backup for the duration of data_ setup, deleted by a signal after setting the data_ to View

public slots:
    void dbStatus(); // info about current DB

signals:
    void processing(bool isProcessing, bool visibleProgress = false);
    void dataUpdated();
    void setStatusbarText(const QString &text = QString()); // text to statusbar
    void setPermanentStatus(const QString &text = QString());
    void showMessage(const QString &text, const QString &title = "Info");
    void cancelProcess();
};

#endif // DATAMAINTAINER_H