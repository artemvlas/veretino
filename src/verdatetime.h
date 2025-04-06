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

    // Verified means all files exist and match the checksums
    enum DT { Created, Updated, Verified };

    // ref to m_created || m_updated || m_verified
    const QString& value(DT type) const;

    void set(DT type, const QString &value);

    // finds type and sets value
    void set(const QString &str);
    void update(DT type);

    // joins stored values into a single string
    QString toString(bool keep_empty_values = true) const;

    // the date until which files are considered unmodified
    QString basicDate() const;

    // returns current dt string, e.g. "Created: 2023/11/09 17:45"
    static QString current(DT type);

    // -- VALUES --
    QString m_created, m_updated, m_verified;
}; // class VerDateTime

#endif // VERDATETIME_H
