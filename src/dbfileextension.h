/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DBFILEEXTENSION_H
#define DBFILEEXTENSION_H

#include <QString>

class DbFileExtension
{
public:
    explicit DbFileExtension(const QString &dbFile);
    QString extension() const;
    bool hasLong() const;
    bool hasShort() const;

    // if <isLong> is true, returns "ver.json" else "ver"
    static QString extension(bool isLong);

    // <dbFile> ends with ".ver.json"
    static bool hasLong(const QString &dbFile);

    // <dbFile> ends with ".ver"
    static bool hasShort(const QString &dbFile);

    // <filePath> ends with ".ver.json" or ".ver"
    static bool isDbFile(const QString &filePath);

    explicit operator bool() const { return isDbFile(*m_dbFile); }

    static const QString s_extLong;  // "ver.json"
    static const QString s_extShort; // "ver"

private:
    const QString *m_dbFile;
}; // class DbFileExtension

#endif // DBFILEEXTENSION_H
