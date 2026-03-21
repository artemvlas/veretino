/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DIGESTSTRING_H
#define DIGESTSTRING_H

#include <QString>
#include <QCryptographicHash>

class DigestString
{
public:
    DigestString(const QString &digest);

    // canBeChecksum
    bool isValid() const;
    bool isValid(QCryptographicHash::Algorithm algo) const;
    static bool isValid(const QString &digest);
    static bool isValid(const QString &digest, QCryptographicHash::Algorithm algo);

    QCryptographicHash::Algorithm algorithm() const;
    static QCryptographicHash::Algorithm algorithm(const QString &digest);

    explicit operator bool() const { return isValid(); }

private:
    const QString *m_digest;
}; // class DigestString

#endif // DIGESTSTRING_H
