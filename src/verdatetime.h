/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef VERDATETIME_H
#define VERDATETIME_H

#include <QString>

class VerDateTime {
public:
    VerDateTime();
    VerDateTime(const QString &str);
    explicit operator bool() const;

    enum DT { Created, Updated, Verified };                             // DateVerified == (all files exist and match the checksums)

    const QString& value(DT type) const;                                // ref to m_created || m_updated || m_verified

    void set(DT type, const QString &value);
    void set(const QString &str);                                       // finds type and sets value
    void update(DT type);

    QString toString(bool keep_empty_values = true) const;              // joins stored values into a single string
    QString basicDate() const;                                          // the date until which files are considered unmodified

    static QString current(DT type);                                    // returns current dt string, e.g. "Created: 2023/11/09 17:45"

    // values
    QString m_created, m_updated, m_verified;
}; // class VerDateTime

#endif // VERDATETIME_H
