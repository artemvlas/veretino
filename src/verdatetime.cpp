/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "verdatetime.h"
#include "tools.h"
#include <QStringList>

const QString VerDateTime::s_dt_format = QStringLiteral(u"yyyy/MM/dd HH:mm");

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

QString VerDateTime::valueWithHint(DT type) const
{
    return hasValue(type) ? tools::joinStrings(valueHint(type), value(type), Lit::s_sepColonSpace)
                          : QString();
}


bool VerDateTime::hasValue(DT type) const
{
    return !value(type).isEmpty();
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
            set((DT)i, cleanValue(dtList.at(i)));
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
            m_created = cleanValue(dt);
            break;
        case 'u':
        case 'U':
            m_updated = cleanValue(dt);
            break;
        case 'v':
        case 'V':
            m_verified = cleanValue(dt);
            break;
        default:
            break;
        }
    }
}

void VerDateTime::update(DT type)
{
    set(type, currentDateTime());
}

void VerDateTime::clear(DT type)
{
    switch (type) {
    case Created:
        m_created.clear();
        break;
    case Updated:
        m_updated.clear();
        break;
    case Verified:
        m_verified.clear();
        break;
    default: break;
    }
}

QString VerDateTime::toString(bool keep_empty_values) const
{
    QString resStr;

    // OLD impl. for values-with-hint stored by default
    /*if (keep_empty_values)
        resStr.reserve(m_created.size() + m_updated.size() + m_verified.size() + 4); // ", " * 2 = 4

    for (int i = 0; i < 3; ++i) {
        const QString &val = value((DT)i);

        if (!val.isEmpty() || keep_empty_values) {
            if (!resStr.isEmpty() || (keep_empty_values && i > 0))
                resStr += Lit::s_sepCommaSpace; // ", "

            resStr += val;
        }
    }*/

    for (int i = 0; i < 3; ++i) {
        if (hasValue((DT)i) || keep_empty_values) {
            if (!resStr.isEmpty() || (keep_empty_values && i > 0))
                resStr += Lit::s_sepCommaSpace; // add ", "

            resStr += valueWithHint((DT)i);
        }
    }

    return resStr;
}

QString VerDateTime::cleanValue(const QString &value_with_hint) const
{
    static const int rLen = s_dt_format.size(); // 16
    return value_with_hint.right(rLen);
}

QString VerDateTime::basicDate() const
{
    if (!m_verified.isEmpty())
        return m_verified;

    if (m_updated.isEmpty())
        return m_created;

    // if the update time is not empty and there is no verification time,
    // return an empty string
    return QString();
}

QString VerDateTime::currentWithHint(DT type)
{
    return tools::joinStrings(valueHint(type), currentDateTime(), Lit::s_sepColonSpace);
}

QString VerDateTime::currentDateTime()
{
    return QDateTime::currentDateTime().toString(s_dt_format);
}

bool VerDateTime::isLater(const QString &dt_before, const QString &dt_later)
{
    // format "yyyy/MM/dd HH:mm"
    const int dt_str_len = 16;

    if (dt_before.size() != dt_str_len
        || dt_later.size() != dt_str_len)
    {
        return false;
    }

    for (int i = 0; i < dt_before.size(); ++i) {
        const int ch1v = dt_before.at(i).digitValue();
        const int ch2v = dt_later.at(i).digitValue();

        if (ch1v < ch2v)
            return true;
        if (ch1v > ch2v)
            return false;
    }

    return false;
}

bool VerDateTime::isLater(const QString &dt_str, const QDateTime &other)
{
    if (dt_str.isEmpty())
        return false;

    return isLater(dt_str, other.toString(s_dt_format));
}

bool VerDateTime::isLater(DT type, const QDateTime &other) const
{
    return isLater(value(type), other);
}

bool VerDateTime::isLater(const QDateTime &other) const
{
    return isLater(basicDate(), other);
}

QString VerDateTime::valueHint(DT type)
{
    switch (type) {
    case Created: return QStringLiteral(u"Created");
    case Updated: return QStringLiteral(u"Updated");
    case Verified: return QStringLiteral(u"Verified");
    default: return QString();
    }
}
