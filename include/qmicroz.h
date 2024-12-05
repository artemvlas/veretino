/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Author: Artem Vlasenko
 * https://github.com/artemvlas
*/

#ifndef QMICROZ_H
#define QMICROZ_H

#include <QtCore/qglobal.h>

#if defined(QMICROZ_LIBRARY)
#define QMICROZ_EXPORT Q_DECL_EXPORT
#else
#define QMICROZ_EXPORT Q_DECL_IMPORT
#endif

#include <QStringList>
#include <QMap>

// path inside zip : data
using BufFileList = QMap<QString, QByteArray>;

struct BufFile {
    BufFile() {}
    BufFile(const QString &filename, const QByteArray &data)
        : m_name(filename), m_data(data) {}

    explicit operator bool() const { return !m_name.isEmpty(); }

    QString m_name;
    QByteArray m_data;
}; // struct BufFile

// list of files (index : path) contained in the archive
using ZipContentsList = QMap<int, QString>;

class QMICROZ_EXPORT QMicroz
{
public:
    QMicroz();
    explicit QMicroz(const char *zip_path);                                            // to avoid ambiguity
    explicit QMicroz(const QString &zip_path);                                         // path to existing zip file
    explicit QMicroz(const QByteArray &buffered_zip);                                  // existing zip archive buffered in memory
    ~QMicroz();

    explicit operator bool() const { return (bool)m_archive; }                         // checks whether the archive is setted
    const ZipContentsList& contents();                                                 // returns a list of files contained in a given archive

    bool setZipFile(const QString &zip_path);                                          // sets and opens the zip for the current object
    bool setZipBuffer(const QByteArray &buffered_zip);                                 // sets a buffered in memory zip archive
    void setOutputFolder(const QString &output_folder);                                // path to the folder where to place the extracted files
    void closeArchive();                                                               // closes the currently setted zip and resets the pointer

    bool extractAll();                                                                 // extracts the archive into the output folder (or the parent one)
    bool extractIndex(int file_index, bool recreate_path = true);                      // extracts the file with index to disk
    bool extractFile(const QString &file_name, bool recreate_path = true);             // find by file_name and extracts if any

    BufFileList extract_to_ram() const;                                                // extracts the archive into the RAM buffer
    BufFile     extract_to_ram(int file_index) const;                                  // extracts the selected index
    BufFile     extract_to_ram_file(const QString &file_name);                         // find by file_name and extracts if any

    // STATIC functions
    static bool extract(const QString &zip_path);                                      // extracting the zip into the parent dir
    static bool extract(const QString &zip_path, const QString &output_folder);        // to output_folder

    static bool compress_(const QString &path);                                        // archiving a file or folder (path), >> parent dir
    static bool compress_(const QStringList &paths);                                   // paths to files or/and folders

    static bool compress_file(const QString &source_path);                             // archiving a file, >> parent dir
    static bool compress_file(const QString &source_path, const QString &zip_path);    // >> zip_path
    static bool compress_folder(const QString &source_path);                           // archiving a folder, >> parent dir
    static bool compress_folder(const QString &source_path, const QString &zip_path);  // >> zip_path
    static bool compress_list(const QStringList &paths, const QString &zip_path);      // archiving a list of files or folders (paths), >> zip_path

    static bool compress_buf(const BufFileList &buf_data, const QString &zip_path);    // creates an archive with files from the listed paths and data
    static bool compress_buf(const QByteArray &data,
                             const QString &file_name, const QString &zip_path);       // creates an archive (zip_path) containing a file (file_name, data)

private:
    const ZipContentsList& updateZipContents();                                        // updates the list of current archive contents
    const QString& outputFolder();                                                     // returns the current folder for extracting the archive
    int findIndex(const QString &file_name);                                           // finds the file index by the specified name

    // the void pointer is used to allow the miniz header not to be included
    void *m_archive = nullptr;

    QString m_zip_path;                                                                // path to the current zip file
    QString m_output_folder;                                                           // folder for extracting the archive
    ZipContentsList m_zip_contents;                                                    // list of current contents { index : filename (or path) }
    static const QString s_zip_ext;

}; // class QMicroz

#endif // QMICROZ_H
