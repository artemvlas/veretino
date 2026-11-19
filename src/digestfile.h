/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/

#ifndef DIGESTFILE_H
#define DIGESTFILE_H

#include <QStringList>
#include <QCryptographicHash>
#include "digest.h"

class DigestFile
{
public:
    explicit DigestFile();
    explicit DigestFile(const QString &mainFile);
    explicit DigestFile(QCryptographicHash::Algorithm algo);
    explicit DigestFile(const Digest &digest);
    explicit DigestFile(const QString &mainFile, QCryptographicHash::Algorithm algo);
    explicit DigestFile(const QString &mainFile, const Digest &digest);

    QCryptographicHash::Algorithm algorithm() const;

    // "md5", "sha1", "sha256", "sha512"
    QString suffix() const;
    static QString suffix(QCryptographicHash::Algorithm algo);
    static QString suffix(int digestLength);

    // <filePath> ends with suffix ("md5", "sha1", "sha256", "sha512")
    static bool isDigestFile(const QString &filePath);

    // ../folder/file.txt --> ../folder/file.txt.shaX
    static QString digestFilePath(const QString &file, QCryptographicHash::Algorithm algo);
    static QString digestFilePath(const QString &file, const int sum_len);

    bool make() const;
    static bool make(const QString &mainFile, const Digest &digest);

    static Digest extractDigest(const QString &digestFile);

    static const QStringList sl_digest_exts;

private:
    QCryptographicHash::Algorithm m_algo = QCryptographicHash::Algorithm(0);
    QString m_mainFile;
    Digest m_digest;
}; // class DigestFile

#endif // DIGESTFILE_H
