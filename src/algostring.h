/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef ALGOSTRING_H
#define ALGOSTRING_H

#include <QStringList>
#include <QCryptographicHash>

class AlgoString
{
public:
    explicit AlgoString(QCryptographicHash::Algorithm algo);
    explicit AlgoString(int digestLength);

    QCryptographicHash::Algorithm algorithm() const;

    // "MD5", "SHA-1", "SHA-256", "SHA-512"
    QString name() const;
    static QString name(QCryptographicHash::Algorithm algo);
    static QString name(int digestLength);

    // "md5", "sha1", "sha256", "sha512"
    QString suffix() const;
    static QString suffix(QCryptographicHash::Algorithm algo);
    static QString suffix(int digestLength);

    // <filePath> ends with suffix ("md5", "sha1", "sha256", "sha512")
    static bool isDigestFile(const QString &filePath);

    // returns the checksum str length: sha(1) = 40, sha(256) = 64, sha(512) = 128
    int digestLength() const;
    static int digestLength(QCryptographicHash::Algorithm algo);

    // 64 -> QCryptographicHash::Sha256
    static QCryptographicHash::Algorithm algoByStrLen(int digestLength);

    // "SHA-256" -> QCryptographicHash::Sha256
    static QCryptographicHash::Algorithm strToAlgo(const QString &strAlgo);

    static const QStringList sl_digest_exts;

private:
    // {0,1,2,3} --> 123
    static int digitsToNum(const QList<int> &digits);

    QCryptographicHash::Algorithm m_algo;

}; // class AlgoString

#endif // ALGOSTRING_H
