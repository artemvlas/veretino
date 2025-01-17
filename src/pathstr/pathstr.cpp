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
        const QChar _ch = path.at(0);
        return _ch.isLetter() ? QStringLiteral(u"Drive_") + _ch.toUpper() : "Root";
    }

    // _sep == '/'
    const bool _endsWithSep = path.endsWith(_sep);
    const int _lastSepInd = path.lastIndexOf(_sep, -2);

    if (_lastSepInd == -1) {
        return _endsWithSep ? path.chopped(1) : path;
    }

    const int _len = _endsWithSep ? (path.size() - _lastSepInd - 2) : -1;
    return path.mid(_lastSepInd + 1, _len);
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
    const int _cut = rootFolder.endsWith(_sep) ? rootFolder.size() - 1 : rootFolder.size();

    return ((_cut < fullPath.size()) && (fullPath.at(_cut) == _sep)) ? fullPath.mid(_cut + 1) : QString();

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

QString suffix(const QString &_file)
{
    const int _len = suffixSize(_file);
    return (_len > 0) ? _file.right(_len).toLower() : QString();
}

QString setSuffix(const QString &_file, const QString &_suf)
{
    //if (hasExtension(_file, _suf))
    //    return _file;

    const int _cur_suf_size = suffixSize(_file);

    if (_cur_suf_size == 0)
        return joinStrings(_file, _suf, _dot);

    QStringView _chopped = QStringView(_file).left(_file.size() - _cur_suf_size);
    return _chopped % _suf;
}

int suffixSize(const QString &_file)
{
    const QString _file_name = basicName(_file); // in case: /folder.22/filename_with_no_dots
    const int _dot_ind = _file_name.lastIndexOf(_dot);

    if (_dot_ind < 1)
        return 0;

    return _file_name.size() - _dot_ind - 1;
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
    const int _dotInd = file.size() - ext.size() - 1;

    return ((_dotInd >= 0 && file.at(_dotInd) == _dot)
            && file.endsWith(ext, Qt::CaseInsensitive));
}

bool hasExtension(const QString &file, const QStringList &extensions)
{
    for (const QString &_ext : extensions) {
        if (hasExtension(file, _ext))
            return true;
    }

    return false;
}

bool isSeparator(const QChar sep)
{
    return (sep == _sep) || (sep == '\\');
}

QString joinStrings(const QString &str1, const QString &str2, QChar sep)
{
    const bool s1Ends = str1.endsWith(sep);
    const bool s2Starts = str2.startsWith(sep);

    if (s1Ends && s2Starts) {
        QStringView _chopped = QStringView(str1).left(str1.size() - 1);
        return _chopped % str2;
    }

    if (s1Ends || s2Starts)
        return str1 + str2;

    return str1 % sep % str2;
}

} // namespace paths
