/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "verdatetime.h"
#include "tools.h"
#include <QStringList>

VerDateTime::VerDateTime()
{}

VerDateTime::VerDateTime(const QString &str)
{
    set(str);
}

VerDateTime::operator bool() const
{
    return (!m_created.isEmpty() || !m_updated.isEmpty());
}

const QString& VerDateTime::value(DT type) const
{
    switch (type) {
    case Created: return m_created;
    case Updated: return m_updated;
    case Verified: return m_verified;
    default: return m_updated;
    }
}

void VerDateTime::set(DT type, const QString &value)
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

void VerDateTime::set(const QString &str)
{
    const QStringList _dtList = str.split(QStringLiteral(u", "));

    if (_dtList.size() == 3) {
        for (int i = 0; i < 3; ++i) {
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

void VerDateTime::update(DT type)
{
    QString __s;

    switch (type) {
    case Created:
        __s = QStringLiteral(u"Created: ");
        break;
    case Updated:
        __s = QStringLiteral(u"Updated: ");
        break;
    case Verified:
        __s = QStringLiteral(u"Verified: ");
        break;
    default:
        return;
    }

    // for example: "Created: 2023/11/09 17:45"
    QString _value = __s + format::currentDateTime();
    set(type, _value);
}

QString VerDateTime::toString(bool keep_empty_values) const
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