#ifndef DATACONTAINER_H
#define DATACONTAINER_H

#include <QObject>
#include <QMap>


/* This class is part of the Veretino project under the GNU GPLv3 license. https://github.com/artemvlas/veretino
 * Objects of this class are used to store, organize, manage database data.
 * For each database, a separate object is created that stores checksums, all lists of files for various needs,
 * info about the algorithm type, relevance, etc. The object can perform basic tasks of sorting, filtering, comparing data.
*/

class DataContainer : public QObject
{
    Q_OBJECT
public:
    explicit DataContainer(const QString &initPath = QString(), QObject *parent = nullptr);
    ~DataContainer();

    QString jsonFilePath;
    QString workDir; // current working folder
    QStringList ignoredExtensions;
    QStringList onlyExtensions;
    int shatype = 0; // 1 or 256 or 512: from json database header or by checksum lenght
    int shaType(); // if ^ is 0 try to compute it by stored checksum string len
    QString lastUpdate; // from "Updated" value of first json object (from header)
    QString storedDataSize; // total size of listed files when db was built
    QMap<QString,QString> mainData;
    QMap<QString,QString> filesAvailability; //contains file : availability status (on Disk or Lost or New)
    QMap<QString,QString> mismatches; //files with failed checksum test
    QMap<QString,QString> recalculated; //checksums recalculated for given filelist (from json database) during verification
    QMap<QString,QString> fillMapSameValues(const QStringList &keys, const QString &value); // create the QMap with multiple keys(QStrinList) and same values
    QStringList lostFiles;
    QStringList newFiles;
    QStringList onDiskFiles;

    //QMap<QString,QString>& defineFilesAvailability();
    QMap<QString,QString> newlostOnly();
    QMap<QString,QString> clearDataFromLostFiles(); // remove lostFiles items from mainData, returns the list of changes
    QMap<QString,QString> updateMainData(const QMap<QString,QString> &listFilesChecksums, const QString &info = "added to DB"); // add calculated checksums to mainData, returns the list of changes

    void setJsonFileNamePrefix(const QString &prefix);
    void setIgnoredExtensions(const QStringList &extensions); // assigns ignoredExtensions, cleares onlyExtensions
    void setOnlyExtensions(const QStringList &extensions); // assigns onlyExtensions, cleares ignoredExtensions

    QString itemContentsInfo(const QString &itemPath); // info about Model item (created with mainData), if file - file info, if folder - folder contents (availability, size etc.)
    QMap<QString,QString> listFolderContents(const QString &rootFolder); // returns a list of files and their availability info in the specified folder from the database

    QString aboutDb(); // info about parsed DB
};

#endif // DATACONTAINER_H
