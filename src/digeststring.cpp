/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#include "digeststring.h"
#include "algostring.h"
#include <QSet>

DigestString::DigestString(const QString &digest)
    : m_digest(&digest) {}

bool DigestString::isValid() const
{
    return isValid(*m_digest);
}

bool DigestString::isValid(QCryptographicHash::Algorithm algo) const
{
    return isValid(*m_digest, algo);
}

bool DigestString::isValid(const QString &digest)
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

bool DigestString::isValid(const QString &digest, QCryptographicHash::Algorithm algo)
{
    return (digest.size() == AlgoString::digestLength(algo)) && isValid(digest);
}

QCryptographicHash::Algorithm DigestString::algorithm() const
{
    return algorithm(*m_digest);
}

QCryptographicHash::Algorithm DigestString::algorithm(const QString &digest)
{
    return AlgoString::algoByStrLen(digest.length());
}
