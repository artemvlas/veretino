/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "digest.h"
#include "algostring.h"
#include <QSet>

DigestView::DigestView(const QString &digest)
    : m_digest(&digest) {}

bool DigestView::isEmpty() const
{
    return m_digest->isEmpty();
}

bool DigestView::isValid() const
{
    return isValid(*m_digest);
}

bool DigestView::isValid(QCryptographicHash::Algorithm algo) const
{
    return isValid(*m_digest, algo);
}

bool DigestView::isValid(const QString &digest)
{
    static const QSet<int> s_perm_length = {32, 40, 64, 128};

    if (!s_perm_length.contains(digest.length())) {
        return false;
    }

    // whether the char is a digit or a letter from 'Aa' to 'Ff'
    auto isHexChar = [](const QChar c) {
        const char ch = c.toLatin1();

        return (ch >= '0' && ch <= '9')
               || (ch >= 'A' && ch <= 'F')
               || (ch >= 'a' && ch <= 'f');
    }; // lambda isHexChar

    for (int i = 0; i < digest.length(); ++i) {
        if (!isHexChar(digest.at(i)))
            return false;
    }

    return true;
}

bool DigestView::isValid(const QString &digest, QCryptographicHash::Algorithm algo)
{
    return (digest.size() == AlgoString::digestLength(algo)) && isValid(digest);
}

bool DigestView::verify(const QString &another) const
{
    return verify(toString(), another);
}

bool DigestView::verify(const DigestView &another) const
{
    return verify(toString(), another.toString());
}

bool DigestView::verify(const QString &first, const QString &second)
{
    if (first.length() != second.length())
        return false;

    for (int i = 0; i < first.length(); ++i) {
        if (first.at(i).toLower() != second.at(i).toLower())
            return false;
    }

    return true;
}

QCryptographicHash::Algorithm DigestView::algorithm() const
{
    return algorithm(*m_digest);
}

QCryptographicHash::Algorithm DigestView::algorithm(const QString &digest)
{
    return AlgoString::algoByStrLen(digest.length());
}
