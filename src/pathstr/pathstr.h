/*
 * A small library for handling filesystem paths as strings.
 *
 * MIT License
 * Author: Artem Vlasenko <artemvlas (at) proton (dot) me>
 *
 * https://github.com/artemvlas
*/
#ifndef PATHSTR_H
#define PATHSTR_H

#include <QString>

namespace pathstr {
static const QChar _sep = u'/';
static const QChar _dot = u'.';

QString basicName(const QString &path);                                       // returns the file or folder name: "/home/user/folder/fname'/'" --> "fname"
QString parentFolder(const QString &path);                                    // "/folder/file_or_folder2'/'" --> "/folder"
QString relativePath(const QString &rootFolder, const QString &fullPath);     // "/folder/rootFolder/folder2/file" --> "folder2/file"
QString shortenPath(const QString &path);                                     // if not a root (or child of root) path, returns "../path"
QString joinPath(const QString &absolutePath, const QString &addPath);        // returns '/absolutePath/addPath'
QString composeFilePath(const QString &parentFolder,
                        const QString &fileName, const QString &ext);
QString suffix(const QString &_file);                                         // "file.txt" --> "txt"
QString setSuffix(const QString &_file, const QString &_suf);                 // "file" or "file.txt" --> "file.zip"
int suffixSize(const QString &_file);                                          // "/folder/file.txt" --> 3

bool isRoot(const QString &path);                                             // true: "/" or "X:'/'"; else false
bool hasExtension(const QString &file, const QString &ext);                   // true if the "file" (name or path) have the "ext" suffix
bool hasExtension(const QString &file, const QStringList &extensions);        // true if the file have any extension from the list

// additional tools
QString joinStrings(const QString &str1, const QString &str2, QChar sep);     // checks for the absence of 'sep' duplication
} // namespace paths

#endif // PATHSTR_H
