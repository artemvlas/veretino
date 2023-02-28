#ifndef FILES_H
#define FILES_H

#include <QObject>

class Files : public QObject
{
    Q_OBJECT
public:
    explicit Files(const QString &path = QString(), QObject *parent = nullptr);

    bool ignoreDbFiles;
    bool ignoreShaFiles;

    QStringList& actualFileList(const QString &folder = QString());
    qint64 filelistSize(const QStringList &filelist);
    QStringList actualFileListFiltered(const QStringList &extensionsList = QStringList(), const QString &folder = QString());
    QStringList filterDbShafiles(const QStringList &filelist);// filtering *.fcc.json or/and *.sha1/256/512 files from filelist
    QStringList filterByExtensions(const QStringList &extensionsList, const QStringList &filelist = QStringList());
    QStringList includedOnlyFilelist(const QStringList &extensionsList, const QString &folder = QString()); // actual filelist with only listed extensions included
    int filesNumber(const QString &folder = QString());
    qint64 folderSize(const QString &folder = QString());
    QString folderContentStatus(const QString &folder = QString());
    QString filelistContentStatus(const QStringList &filelist);
    QString filesNumberSizeToReadable(const int &filesNumber, const qint64 &filesSize);
    QString fileNameSize(const QString &path = QString()); // returns "filename (readable size)"
    QString filePath;
    QString folderPath;
    //QStringList fileList;
    QStringList actualFiles; // all files contained in the folder and its subfolders
    QStringList filteredFiles; // list of filtered files created by filterByExtensions() function

public slots:
    void processFileList(const QString &rootFolder);

signals:
    //void completeList(const QStringList &list);
    //void totalSize(const qint64 &asize);
    void errorMessage(const QString &text);
};

#endif // FILES_H
