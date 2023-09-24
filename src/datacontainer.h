/* This file is part of the Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
 * These classes are used to store, organize, manage the data.
 * For each database, a separate object is created that stores checksums, all lists of files for various needs,
 * info about the algorithm type, relevance, etc. Objects can perform basic tasks of sorting, filtering, comparing data.
*/
#ifndef DATACONTAINER_H
#define DATACONTAINER_H

#include <QObject>
#include <QMap>
#include <QCryptographicHash>
#include "files.h"
#include "treemodel.h"

class TreeModel;

struct MetaData {
    QCryptographicHash::Algorithm algorithm;
    //int shaType = 0; // 1 or 256 or 512
    QString workDir; // current working folder
    QString databaseFileName;
    QString saveDateTime; // date and time the database was saved
    QString about; // contains a brief description of the item changes or status, if any
    QString totalSize;
    FilterRule filter;
};

struct Numbers {
    int numChecksums = 0; // number of files with checksums
    int numMatched = 0; // number of check files with matched checksums
    int numMismatched = 0; // ... mismatched checksums
    int numAvailable = 0; // the number of files that exist on the disk and are readable, for which checksums are stored
    int numNewFiles = 0;
    int numMissingFiles = 0;
    int numUnreadable = 0;
    int numNotChecked = 0;

    qint64 totalSize = 0; // total size in bytes of all files for which there are checksums in 'filesData'
};

struct DataContainer {
    MetaData metaData;
    Numbers numbers;
    FileList filesData; // main data

    DataContainer(){}
    DataContainer(const MetaData &metadata) : metaData(metadata){}
};

class DataMaintainer : public QObject
{
    Q_OBJECT
public:
    explicit DataMaintainer(QObject *parent = nullptr);
    explicit DataMaintainer(const DataContainer &initData, QObject *parent = nullptr);
    ~DataMaintainer();

    // functions
    void updateMetaData();
    void updateNumbers();

    int updateData(const FileList &updateFiles); // add new data to 'data_.filesData', returns number of added/updated items: updateFiles.size()
    int updateData(FileList updateFiles, FileValues::FileStatus status); // adding 'status' to 'updateFiles' and updateData(updateFiles)^
    bool updateData(const QString &filePath, const QString &checksum); // update the check status of the file

    int clearDataFromLostFiles(); // returns the number of cleared
    int updateMismatchedChecksums(); // returns the number of updated checksums

    FileList listOf(FileValues::FileStatus fileStatus); // list of files with specified status
    FileList listOf(FileValues::FileStatus fileStatus, const FileList &originalList);
    FileList listOf(QSet<FileValues::FileStatus> fileStatuses); // list of multiple conditions^

    void importJson(const QString &jsonFilePath);
    void exportToJson();

    FileList subfolderContent(QString subFolder);
    QString itemContentsInfo(const QString &itemPath); // if file - file info, if folder - folder contents (availability, size etc.)

    void dbStatus(); // info about current DB

    // variables
    DataContainer data_;
    TreeModel *model_ = nullptr;

public slots:
    void cancelProcess();
private:
    bool canceled = false;
    int shaType(const FileList &fileList); // determines the shaType by the string length of the stored checksum
    int findNewFiles(); // Searches for new readable files regarding stored list and filters, returns the number of found
    void updateFilesValues();

signals:
    void setStatusbarText(const QString &text = QString()); // text to statusbar
    void setPermanentStatus(const QString &text = QString());
    void showMessage(const QString &text, const QString &title = "Info");    
};

#endif // DATACONTAINER_H
