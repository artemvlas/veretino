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

    // ref to mCreated || mUpdated || mVerified
    const QString& value(DT type) const;

    // e.g. "Created: 2024/09/24 18:35"
    QString valueWithHint(DT type) const;

    // is the corresponding value not empty
    bool hasValue(DT type) const;

    // e.g. (DT::Created, "2024/09/24 18:35")
    void set(DT type, const QString &value);

    /* parse 'str' and set found values
     * e.g. str == "Created: 2023/11/03 19:15, Updated: 2024/05/02 17:13, Verified: 2025/09/03 21:37" --->>
     * mCreated = "2023/11/03 19:15"
     * mUpdated = "2024/05/02 17:13"
     * mVerified = "2025/09/03 21:37"
     */
    void set(const QString &str);

    // set current datetime to specified value
    void update(DT type);

    // clear value
    void clear(DT type);

    /* join stored values to a single string with hints
     * e.g. mCreated = "2023/11/03 19:15", mUpdated = "2024/05/02 17:13" --->>
     * "Created: "2023/11/03 19:15", Updated: "2024/05/02 17:13"
     */
    QString toString(bool keep_empty_values = true) const;

    // the date until which files are considered unmodified
    QString basicDate() const;

    // returns current dt string, e.g. "Created: 2023/11/09 17:45"
    static QString currentWithHint(DT type);

private:
    // "Created: 2024/09/24 18:35" --> "2024/09/24 18:35"
    QString cleanValue(const QString &value_with_hint) const;

    static QString valueHint(DT type);

    /*** VALUES ***/
    // format: "2024/09/24 18:35"
    QString mCreated, mUpdated, mVerified;

}; // class VerDateTime

#endif // VERDATETIME_H
