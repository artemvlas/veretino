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
    bool isChecked = false; // Has the data been verified?
    int shaType = 0; // 1 or 256 or 512
    int numChecksums = 0; // number of files with checksums
    int numMatched = 0; // number of check files with matched checksums
    int numMismatched = 0; // ... mismatched checksums
    int numAvailable = 0; // the number of files that exist on the disk and are readable, for which checksums are stored
    int numNewFiles = 0;
    int numMissingFiles = 0;
    int numUnreadable = 0;
    QString workDir; // current working folder
    QString databaseFileName;
    QString saveDateTime; // date and time the database was saved
    QString about; // contains a brief description of the item changes or status, if any
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

    int updateData(const FileList &updateFiles); // add new data to 'data_.filesData', returns number of added/updated items: updateFiles.size()
    int updateData(FileList updateFiles, FileValues::FileStatus status); // adding 'status' to 'updateFiles' and updateData(updateFiles)^
    bool updateData(const QString &filePath, const QString &checksum); // update the check status of the file

    void updateFilesValues();
    int findNewFiles(); // Searches for new readable files regarding stored list and filters, returns the number of found

    int clearDataFromLostFiles(); // returns the number of cleared
    int updateMismatchedChecksums(); // returns the number of updated checksums

    enum Listing {Available, New, Lost, Changes, Mismatches, Added, Removed, Updated};
    FileList listOf(Listing only);
    FileList listOf(QList<Listing> only); // list of multiple conditions^

    void importJson(const QString &jsonFilePath);
    void exportToJson();

    QString itemContentsInfo(const QString &itemPath); // info about Model item (created with mainData), if file - file info, if folder - folder contents (availability, size etc.)
    FileList listFolderContents(QString rootFolder); // returns a list of files and their availability info in the specified folder from the database

    void dbStatus(); // info about current DB

    // variables
    DataContainer data_;

public slots:
    void cancelProcess();
private:
    bool canceled = false;
    int shaType(const FileList &fileList); // determines the shaType by the string length of the stored checksum

signals:
    void statusChanged(const QString &text = QString()); // text to statusbar
    void showMessage(const QString &text, const QString &title = "Info");
    void setPermanentStatus(const QString &text = QString());
};

#endif // DATACONTAINER_H
