/* This file is part of the Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
 * These classes are used to store, organize, manage the data.
 * For each database, a separate object is created that stores checksums, all lists of files for various needs,
 * info about the algorithm type, relevance, etc. Objects can perform basic tasks of sorting, filtering, comparing data.
*/
#ifndef DATACONTAINER_H
#define DATACONTAINER_H

#include <QObject>
#include <QMap>
#include "files.h"

struct MetaData {
    int shaType = 0; // 1 or 256 or 512
    int numChecksums = 0; // number of files with checksums
    int numNewFiles = 0;
    int numMissingFiles = 0;
    QString workDir; // current working folder
    QString databaseFileName;
    QString saveDateTime; // date and time the database was saved
    QString about; // contains a brief description of the item changes or status, if any
    QString storedTotalSize; // imported from database value
    qint64 totalSize = 0; // total size of all files for which there are checksums in 'filesData'
    FilterRule filter;
};

struct DataContainer {
    MetaData metaData;
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

    void updateData(const FileList &updateFiles); // add new data to 'data_.filesData'

    void updateFilesValues();
    void findNewFiles(); // Searches for new readable files regarding stored list and filters

    void clearDataFromLostFiles();
    void updateMismatchedChecksums();

    DataContainer availableFiles(); // returns a list of available (existing on disk and readable) files from the stored database list ('data_.filesData')
    DataContainer newFiles(); // returns a list of files marked as new (isNew = true) from the 'dataContainer.filesData'
    FileList newlostOnly();
    FileList changesOnly();

    void importJson(const QString &jsonFilePath);
    void exportToJson();

    QString itemContentsInfo(const QString &itemPath); // info about Model item (created with mainData), if file - file info, if folder - folder contents (availability, size etc.)
    FileList listFolderContents(QString rootFolder); // returns a list of files and their availability info in the specified folder from the database

    QString aboutDb(); // info about current DB

    // variables
    DataContainer data_;

public slots:
    void cancelProcess();
private:
    bool canceled = false;
    int shaType(const FileList &fileList); // determines the shaType by the string length of the stored checksum

signals:
    void status(const QString &text = QString()); // text to statusbar
    void showMessage(const QString &text, const QString &title = "Info");
};

#endif // DATACONTAINER_H
