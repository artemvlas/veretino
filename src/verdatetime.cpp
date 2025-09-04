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
    const QStringList dtList = str.split(Lit::s_sepCommaSpace); // ", "

    if (dtList.size() == 3) {
        for (int i = 0; i < 3; ++i) {
            set((DT)i, dtList.at(i));
        }
        return;
    }

    for (const QString &dt : dtList) {
        if (dt.isEmpty())
            continue;

        const char ch = dt.front().toLatin1();

        switch (ch) {
        case 'c':
        case 'C':
            m_created = dt;
            break;
        case 'u':
        case 'U':
            m_updated = dt;
            break;
        case 'v':
        case 'V':
            m_verified = dt;
            break;
        default:
            break;
        }
    }
}

void VerDateTime::update(DT type)
{
    set(type, current(type));
}

QString VerDateTime::toString(bool keep_empty_values) const
{
    QString resStr;

    if (keep_empty_values)
        resStr.reserve(m_created.size() + m_updated.size() + m_verified.size() + 4); // ", " * 2 = 4

    for (int i = 0; i < 3; ++i) {
        const QString &val = value((DT)i);

        if (!val.isEmpty() || keep_empty_values) {
            if (!resStr.isEmpty() || (keep_empty_values && i > 0))
                resStr += Lit::s_sepCommaSpace; // ", "

            resStr += val;
        }
    }

    return resStr;
}

QString VerDateTime::cleanValue(DT type) const
{
    // "Created: 2024/09/24 18:35" --> "2024/09/24 18:35"
    const int rLen = Lit::s_dt_format.size(); // 16
    return value(type).right(rLen);
}

QString VerDateTime::basicDate() const
{
    // "Created: 2024/09/24 18:35" --> "2024/09/24 18:35"
    const int r_len = Lit::s_dt_format.size(); // 16

    if (!m_verified.isEmpty())
        return m_verified.right(r_len);

    if (m_updated.isEmpty())
        return m_created.right(r_len);

    // if the update time is not empty and there is no verification time,
    // return an empty string
    return QString();
}

QString VerDateTime::current(DT type)
{
    QString str;

    switch (type) {
    case Created:
        str = QStringLiteral(u"Created: ");
        break;
    case Updated:
        str = QStringLiteral(u"Updated: ");
        break;
    case Verified:
        str = QStringLiteral(u"Verified: ");
        break;
    default:
        break;
    }

    // for example: "Created: 2023/11/09 17:45"
    return str + format::currentDateTime();
}
