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

    // DateVerified == (all files exist and match the checksums)
    enum DT { Created, Updated, Verified };

    const QString& value(DT type) const;

    void set(DT type, const QString &value);
    void set(const QString &str);
    void update(DT type);

    QString toString(bool keep_empty_values = true) const;

    QString m_created, m_updated, m_verified;
}; // struct VerDateTime

#endif // VERDATETIME_H
