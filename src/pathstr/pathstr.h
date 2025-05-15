/*
 * A small library for handling filesystem paths as strings.
 *
 * MIT License
 * Copyright (c) 2021 - present Artem Vlasenko
 *
 * <artemvlas (at) proton (dot) me>
 * https://github.com/artemvlas
*/
#ifndef PATHSTR_H
#define PATHSTR_H

#include <QString>

namespace pathstr {
static const QChar _sep = u'/';
static const QChar _dot = u'.';

// returns the file or folder name: "/home/user/folder/fname'/'" --> "fname"
QString basicName(const QString &path);

// "/folder/file_or_folder2'/'" --> "/folder"
QString parentFolder(const QString &path);

// "/folder/rootFolder/folder2/file" --> "folder2/file"
QString relativePath(const QString &rootFolder, const QString &fullPath);

// if not a root (or child of root) path, returns "../path"
QString shortenPath(const QString &path);

// returns '/absolutePath/addPath'
QString joinPath(const QString &absolutePath, const QString &addPath);

// parentFolder/fileName.ext
QString composeFilePath(const QString &parentFolder,
                        const QString &fileName, const QString &ext);

// "/home/folder" --> "/"; "C:/Folder" --> "C:/"
QString root(const QString &path);

// "file.txt" --> "txt"
QString suffix(const QString &file);

// "file" or "file.txt" --> "file.zip"
QString setSuffix(const QString &file, const QString &suf);

// "/folder/file.txt" --> 3
int suffixSize(const QString &file);

// true: "/" or "X:[/]"; else false
bool isRoot(const QString &path);

// true if the file (name or path) have the "ext-value" suffix
bool hasExtension(const QString &file, const QString &ext);

// true if the file have any extension from the list
bool hasExtension(const QString &file, const QStringList &extensions);

// true if '/' or '\\'
bool isSeparator(const QChar sep);

// path string ends with a slash ('/' or '\\')
bool endsWithSep(const QString &path);

/*** additional tools ***/
// checks for the absence of 'sep' duplication
QString joinStrings(const QString &str1, const QString &str2, QChar sep);
} // namespace pathstr

#endif // PATHSTR_H
