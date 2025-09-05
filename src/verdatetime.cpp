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
    return (!mCreated.isEmpty() || !mUpdated.isEmpty());
}

const QString& VerDateTime::value(DT type) const
{
    switch (type) {
    case Created: return mCreated;
    case Updated: return mUpdated;
    case Verified: return mVerified;
    default: return mUpdated;
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
        mCreated = value;
        mUpdated.clear();
        mVerified.clear();
        break;
    case Updated:
        mUpdated = value;
        mVerified.clear();
        break;
    case Verified:
        mVerified = value;
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
            mCreated = cleanValue(dt);
            break;
        case 'u':
        case 'U':
            mUpdated = cleanValue(dt);
            break;
        case 'v':
        case 'V':
            mVerified = cleanValue(dt);
            break;
        default:
            break;
        }
    }
}

void VerDateTime::update(DT type)
{
    set(type, format::currentDateTime());
}

void VerDateTime::clear(DT type)
{
    switch (type) {
    case Created:
        mCreated.clear();
        break;
    case Updated:
        mUpdated.clear();
        break;
    case Verified:
        mVerified.clear();
        break;
    default: break;
    }
}

QString VerDateTime::toString(bool keep_empty_values) const
{
    QString resStr;

    // OLD impl. for values-with-hint stored by default
    /*if (keep_empty_values)
        resStr.reserve(mCreated.size() + mUpdated.size() + mVerified.size() + 4); // ", " * 2 = 4

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
    static const int rLen = Lit::s_dt_format.size(); // 16
    return value_with_hint.right(rLen);
}

QString VerDateTime::basicDate() const
{
    if (!mVerified.isEmpty())
        return mVerified;

    if (mUpdated.isEmpty())
        return mCreated;

    // if the update time is not empty and there is no verification time,
    // return an empty string
    return QString();
}

QString VerDateTime::currentWithHint(DT type)
{
    QString str;

    /*switch (type) {
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
    return str + format::currentDateTime();*/

    return tools::joinStrings(valueHint(type), format::currentDateTime(), Lit::s_sepColonSpace);
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
