#include "files.h"
#include <QDirIterator>
#include <QDebug>
#include <cmath>

Files::Files(QObject *parent)
    : QObject{parent}
{}

Files::Files(const QString &initPath, QObject *parent)
    : QObject{parent}
{
    if (QFileInfo(initPath).isFile())
        initFilePath = initPath;
    else if (QFileInfo(initPath).isDir())
        initFolderPath = initPath;
}

Files::Files(const QStringList &fileList, QObject *parent)
    : QObject{parent}, initFileList(fileList)
{}

QString Files::parentFolder()
{
    return parentFolder(initFilePath);
}

QString Files::parentFolder(const QString &path)
{
    int rootSepIndex = path.indexOf('/'); // index of root '/': 0 for '/home/folder'; 2 for 'C:/folder'
    if (rootSepIndex == -1)
        return "/"; // if there is no '/' in 'path', return '/' as root

    if (path.at(rootSepIndex + 1) == '/')
        ++rootSepIndex; // if the path is like 'ftp://folder' or 'smb://folder', increase index to next position
    int sepIndex = path.lastIndexOf('/', -2); // skip the last char due the case /home/folder'/'
    if (sepIndex > rootSepIndex)
        return path.left(sepIndex);
    else
        return path.left(rootSepIndex + 1); // if the last 'sep' is also the root, keep it
}

QString Files::contentStatus()
{
    if (initFilePath != nullptr)
        return contentStatus(initFilePath);
    else if (initFolderPath != nullptr)
        return contentStatus(initFolderPath);
    else if (!initFileList.isEmpty())
        return contentStatus(initFileList);
    else
        return QString();
}

QString Files::contentStatus(const QString &path)
{
    QFileInfo fInfo(path);
    if (fInfo.isFile())
        return QString("%1 (%2)").arg(fInfo.fileName(), dataSizeReadable(fInfo.size()));
    else if (fInfo.isDir())
        return QString("%1: %2").arg(QDir(path).dirName(), contentStatus(allFiles(path)));
    else
        return QString();
}

QString Files::contentStatus(const QStringList &filelist)
{
    return contentStatus(filelist.size(), dataSize(filelist));
}

QString Files::contentStatus(const int &filesNumber, const qint64 &filesSize)
{
    QChar s; // if only 1 file - text is "file", if more - text is "files"
    if (filesNumber != 1)
        s = 's';

    return QString("%1 file%2 (%3)").arg(filesNumber).arg(s).arg(dataSizeReadable(filesSize));
}

QString Files::folderContentsByType(const QString &folder)
{
    QStringList files = allFiles(folder);

    if (files.isEmpty())
        return "Empty folder";

    QHash<QString, QStringList> listsByType; // key = extension, value = list of files with that extension

    foreach (const QString &file, files) {
        QString ext = QFileInfo(file).suffix().toLower();
        if (ext == "")
            ext = "No type";
        listsByType[ext].append(file);
    }

    struct combinedByType {
        QString extension;
        QStringList filelist;
        qint64 filesSize;
    };

    QList<combinedByType> combList;

    foreach (const QString &ext, listsByType.keys()) {
        combinedByType t;
        t.extension = ext;
        t.filelist = listsByType.value(ext);
        t.filesSize = dataSize(t.filelist);
        combList.append(t);
    }

    std::sort(combList.begin(), combList.end(), [](const combinedByType &t1, const combinedByType &t2) {return (t1.filesSize > t2.filesSize);});

    QString text;
    if (combList.size() > 10) {
        text.append(QString("Top sized file types:\n%1\n").arg(QString('-').repeated(30)));
        qint64 excSize = 0; // total size of files whose types are not displayed
        int excNumber = 0; // the number of these files
        for (int var = 0; var < combList.size(); ++var) {
            if (var < 10) {
                text.append(QString("%1: %2\n").arg(combList.at(var).extension, contentStatus(combList.at(var).filelist)));
            }
            else {
                excSize += combList.at(var).filesSize;
                excNumber += combList.at(var).filelist.size();
            }
        }
        text.append(QString("...\nOther %1 types: %2\n").arg(combList.size() - 10).arg(contentStatus(excNumber, excSize)));
    }
    else {
        foreach (const combinedByType &t, combList) {
            text.append(QString("%1: %2\n").arg(t.extension, contentStatus(t.filelist)));
        }
    }

    QString totalInfo = QString(" Total: %1 types, %2").arg(combList.size()).arg(contentStatus(files));
    text.append(QString("%1\n%2").arg(QString('_').repeated(totalInfo.length()), totalInfo));

    return text;
}

QStringList Files::filteredFileList(QStringList extensionsList, const bool includeOnly, const QStringList &filelist)
{
    QStringList files;
    QStringList filteredFiles; // result list

    if (filelist != QStringList())
        files = filelist;
    else
        files = allFiles();

    if (!includeOnly && ignoreDbFiles)
        extensionsList.append("ver.json");
    if (!includeOnly && ignoreShaFiles)
        extensionsList.append({"sha1", "sha256", "sha512"});

    if (extensionsList.isEmpty()) {
        qDebug() << "Files::filteredFileList | 'extensionsList' is Empty. Original list returned";
        return files;
    }

    foreach (const QString &file, files) {
        // to be able to filter compound extensions (like *.ver.json), a comparison loop is used instead of
        // 'extensionsList.contains(QFileInfo(file).suffix().toLower())'
        bool allowed = !includeOnly;
        foreach (const QString &ext, extensionsList) {
            if (file.endsWith('.' + ext, Qt::CaseInsensitive)) {
                allowed = includeOnly;
                break;
            }
        }
        if (allowed)
            filteredFiles.append(file);
    }

    return filteredFiles;
}

QStringList& Files::allFiles()
{
    if (allFilesList.isEmpty())
        allFilesList = allFiles(initFolderPath);

    return allFilesList;
}

QStringList Files::allFiles(const QString &rootFolder)
{
    if (rootFolder == nullptr) {
        return allFiles();
    }
    if (!QFileInfo(rootFolder).isDir()) {
        qDebug() << "Files::allFiles | Not a folder path: " << rootFolder;
        return QStringList();
    }

    QStringList fileList;
    QDirIterator it(rootFolder, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        fileList.append(it.next());

    return fileList;
}

QString Files::joinPath(const QString &addPath)
{
    return joinPath(initFolderPath, addPath);
}

QString Files::joinPath(const QString &absolutePath, const QString &addPath)
{
    QChar sep;
    if (!absolutePath.endsWith('/'))
        sep = '/';

    return QString("%1%2%3").arg(absolutePath, sep, addPath);
}

qint64 Files::dataSize()
{
    if (initFolderPath != nullptr)
        return dataSize(allFiles());
    else if (!initFileList.isEmpty())
        return dataSize(initFileList);
    else {
        qDebug() << "Files::dataSize() | No data to size return";
        return 0;
    }
}

qint64 Files::dataSize(const QString &folder)
{
    return dataSize(allFiles(folder));
}

qint64 Files::dataSize(const QStringList &filelist)
{
    qint64 totalSize = 0;

    if (!filelist.isEmpty()) {
        foreach (const QString &file, filelist) {
            totalSize += QFileInfo(file).size();
        }
    }

    return totalSize;
}

QString Files::dataSizeReadable(const qint64 &sizeBytes)
{
    long double converted = sizeBytes;
    QString xB;

    if (converted > 1024) {
        converted /= 1024;
        xB = "KiB";
        if (converted > 1024) {
            converted /= 1024;
            xB = "MiB";
            if (converted > 1024) {
                converted /= 1024;
                xB = "GiB";
                if (converted > 1024) {
                    converted /= 1024;
                    xB = "TiB";
                }
            }
        }

        float x = std::round(converted * 100) / 100;
        return QString("%1 %2").arg(QString::number(x, 'f', 2), xB);
    }
    else
        return QString("%1 bytes").arg(sizeBytes);
}
