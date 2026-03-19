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
    DbFileExtension(const QString &dbFile);
    QString extension() const;
    bool hasLong() const;
    bool hasShort() const;

    static QString extension(bool isLong);
    static bool hasLong(const QString &dbFile);
    static bool hasShort(const QString &dbFile);

    static const QString s_extLong;
    static const QString s_extShort;

private:
    QString m_dbFile;
}; // class DbFileExtension

#endif // DBFILEEXTENSION_H
