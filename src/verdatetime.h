/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef VERDATETIME_H
#define VERDATETIME_H

#include <QString>

struct VerDateTime {
    VerDateTime() {}
    VerDateTime(const QString &str) { set(str); }

    explicit operator bool() const { return (!m_created.isEmpty() || !m_updated.isEmpty()); }

    // DateVerified == (all files exist and match the checksums)
    enum DT { Created, Updated, Verified };
    const QString& value(DT type) const
    {
        switch (type) {
        case Created: return m_created;
        case Updated: return m_updated;
        case Verified: return m_verified;
        }
    }

    void set(DT type, const QString &value)
    {
        switch (type) {
        case Created:
            m_created = value;
            m_updated.clear();
            m_verified.clear();
            break;
        case Updated:
            m_updated = value;
            m_verified.clear();
            break;
        case Verified:
            m_verified = value;
        }
    }

    void set(const QString &str)
    {
        const QStringList _dtList = str.split(QStringLiteral(u", "));

        if (_dtList.size() == 3) {
            for (int i = 0; i < _dtList.size(); ++i) { // i < 3
                set((DT)i, _dtList.at(i));
            }
        } else {
            for (const QString &_dt : _dtList) {
                if (_dt.isEmpty())
                    continue;

                const char _c = _dt.front().toLatin1();

                switch (_c) {
                case 'c':
                case 'C':
                    m_created = _dt;
                    break;
                case 'u':
                case 'U':
                    m_updated = _dt;
                    break;
                case 'v':
                case 'V':
                    m_verified = _dt;
                    break;
                default:
                    break;
                }
            }
        }
    }

    QString toString(bool keep_empty_values = true) const
    {
        QString __s;

        if (keep_empty_values)
            __s.reserve(m_created.size() + m_updated.size() + m_verified.size() + 4); // ", " * 2 = 4

        for (int i = 0; i < 3; ++i) {
            const QString &__v = value((DT)i);

            if (!__v.isEmpty() || keep_empty_values) {
                if (!__s.isEmpty() || (keep_empty_values && i > 0))
                    __s += QStringLiteral(u", ");

                __s += __v;
            }
        }

        return __s;
    }

    QString m_created, m_updated, m_verified;
}; // struct VerDateTime


#endif // VERDATETIME_H
