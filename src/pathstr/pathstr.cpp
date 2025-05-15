/*
 * A small library for handling filesystem paths as strings.
 *
 * MIT License
 * Copyright (c) 2021 - present Artem Vlasenko
 *
 * <artemvlas (at) proton (dot) me>
 * https://github.com/artemvlas
*/
#include "pathstr.h"
#include <QStringBuilder>
#include <QStringList>

namespace pathstr {
QString basicName(const QString &path)
{
    if (isRoot(path)) {
        const QChar ch = path.at(0);
        return ch.isLetter() ? QStringLiteral(u"Drive_") + ch.toUpper() : "Root";
    }

    // _sep == '/'
    const bool endsWithSep = path.endsWith(_sep);
    const int lastSepInd = path.lastIndexOf(_sep, -2);

    if (lastSepInd == -1) {
        return endsWithSep ? path.chopped(1) : path;
    }

    const int len = endsWithSep ? (path.size() - lastSepInd - 2) : -1;
    return path.mid(lastSepInd + 1, len);
}

QString parentFolder(const QString &path)
{
    const int ind = path.lastIndexOf(_sep, -2);

    switch (ind) {
    case -1: // root --> root; string'/' --> ""
        return isRoot(path) ? path : QString();
    case 0: // /folder'/' --> "/"
        return path.at(ind);
    case 2: // C:/folder'/' --> "C:/"
        return isRoot(path.left(ind)) ? path.left(3) : path.left(ind);
    default: // /folder/item'/' --> /folder
        return path.left(ind);
    }
}

QString relativePath(const QString &rootFolder, const QString &fullPath)
{
    if (rootFolder.isEmpty())
        return fullPath;

    if (!fullPath.startsWith(rootFolder))
        return QString();

    // _sep == u'/';
    const int cut = rootFolder.endsWith(_sep) ? rootFolder.size() - 1 : rootFolder.size();

    return ((cut < fullPath.size()) && (fullPath.at(cut) == _sep)) ? fullPath.mid(cut + 1) : QString();

    // #2 impl. --> x2 slower due to (rootFolder + '/')
    // const QString &_root = rootFolder.endsWith('/') ? rootFolder : rootFolder + '/';
    // return fullPath.startsWith(_root) ? fullPath.mid(_root.size()) : QString();
}

QString shortenPath(const QString &path)
{
    return isRoot(parentFolder(path)) ? path
                                      : QStringLiteral(u"../") + basicName(path);
}

QString joinPath(const QString &absolutePath, const QString &addPath)
{
    return joinStrings(absolutePath, addPath, _sep);
}

QString composeFilePath(const QString &parentFolder, const QString &fileName, const QString &ext)
{
    // with sep check
    // const QString _file = joinStrings(fileName, ext, u'.');
    // return joinPath(parentFolder, _file);

    // no sep check
    return parentFolder % _sep % fileName % _dot % ext;
}

QString root(const QString &path)
{
    // Unix-style fs root "/"
    if (path.startsWith(_sep))
        return _sep;

    // Windows-style root "C:/"
    if (path.size() > 1
        && path.at(0).isLetter()
        && path.at(1) == u':')
    {
        if (path.size() == 2)    // "C:"
            return path + _sep;  // --> "C:/"

        if (isSeparator(path.at(2)))
            return path.left(3);
    }

    // no root found
    return QString();
}

QString suffix(const QString &file)
{
    const int len = suffixSize(file);
    return (len > 0) ? file.right(len).toLower() : QString();
}

QString setSuffix(const QString &file, const QString &suf)
{
    //if (hasExtension(_file, _suf))
    //    return _file;

    const int cur_suf_size = suffixSize(file);

    if (cur_suf_size == 0)
        return joinStrings(file, suf, _dot);

    QStringView chopped = QStringView(file).left(file.size() - cur_suf_size);
    return chopped % suf;
}

int suffixSize(const QString &file)
{
    const QString file_name = basicName(file); // in case: /folder.22/filename_with_no_dots
    const int dot_ind = file_name.lastIndexOf(_dot);

    if (dot_ind < 1)
        return 0;

    return file_name.size() - dot_ind - 1;
}

bool isRoot(const QString &path)
{
    switch (path.length()) {
    case 1:
        return (path.at(0) == _sep); // Linux FS root
    case 2:
    case 3:
        return (path.at(0).isLetter() && path.at(1) == u':'); // Windows drive root
    default:
        return false;
    }
}

bool hasExtension(const QString &file, const QString &ext)
{
    const int dotInd = file.size() - ext.size() - 1;

    return ((dotInd >= 0 && file.at(dotInd) == _dot)
            && file.endsWith(ext, Qt::CaseInsensitive));
}

bool hasExtension(const QString &file, const QStringList &extensions)
{
    for (const QString &ext : extensions) {
        if (hasExtension(file, ext))
            return true;
    }

    return false;
}

bool isSeparator(const QChar sep)
{
    return (sep == _sep) || (sep == '\\');
}

bool endsWithSep(const QString &path)
{
    return !path.isEmpty() && isSeparator(path.back());
}

QString joinStrings(const QString &str1, const QString &str2, QChar sep)
{
    const bool s1Ends = str1.endsWith(sep);
    const bool s2Starts = str2.startsWith(sep);

    if (s1Ends && s2Starts) {
        QStringView chopped = QStringView(str1).left(str1.size() - 1);
        return chopped % str2;
    }

    if (s1Ends || s2Starts)
        return str1 + str2;

    return str1 % sep % str2;
}

} // namespace pathstr
