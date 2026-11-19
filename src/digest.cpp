/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "digest.h"
#include "algostring.h"
#include <QSet>

DigestView::DigestView(const QByteArray &digest)
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

bool DigestView::isValid(const QByteArray &digest)
{    
    switch (digest.length()) {
    case 32:
    case 40:
    case 64:
    case 128:
        break;
    default:
        return false;
    }

    // whether the char is a digit or a letter from 'Aa' to 'Ff'
    auto isHexChar = [](const char ch) {
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

bool DigestView::isValid(const QByteArray &digest, QCryptographicHash::Algorithm algo)
{
    return (digest.size() == AlgoString::digestLength(algo)) && isValid(digest);
}

bool DigestView::verify(const QByteArray &another) const
{
    return verify(digest(), another);
}

bool DigestView::verify(const DigestView &another) const
{
    return verify(digest(), another.digest());
}

bool DigestView::verify(const QByteArray &first, const QByteArray &second)
{
    if (first.length() != second.length())
        return false;

    for (int i = 0; i < first.length(); ++i) {
        QChar ch1 = first.at(i);
        QChar ch2 = second.at(i);

        if (ch1.toLower() != ch2.toLower())
            return false;
    }

    return true;
}

QCryptographicHash::Algorithm DigestView::algorithm() const
{
    return algorithm(*m_digest);
}

QCryptographicHash::Algorithm DigestView::algorithm(const QByteArray &digest)
{
    return AlgoString::algoByStrLen(digest.length());
}
