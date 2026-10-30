/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/

#include "digestfile.h"
#include "pathstr.h"
#include "algostring.h"
#include "tools.h"
#include "exception.h"
#include <QDebug>

const QStringList DigestFile::sl_digest_exts = {
    QStringLiteral(u"md5"),
    QStringLiteral(u"sha1"),
    QStringLiteral(u"sha256"),
    QStringLiteral(u"sha512")
};

DigestFile::DigestFile() {}

DigestFile::DigestFile(const QString &mainFile)
    : m_mainFile(mainFile) {}

DigestFile::DigestFile(QCryptographicHash::Algorithm algo)
    : m_algo(algo) {}

DigestFile::DigestFile(const Digest &digest)
    : m_digest(digest) {}

DigestFile::DigestFile(const QString &mainFile, QCryptographicHash::Algorithm algo)
    : m_mainFile(mainFile), m_algo(algo) {}

DigestFile::DigestFile(const QString &mainFile, const Digest &digest)
    : m_mainFile(mainFile), m_digest(digest) {}

QCryptographicHash::Algorithm DigestFile::algorithm() const
{
    return m_algo;
}

QString DigestFile::suffix() const
{
    if (!m_algo)
        qWarning() << Q_FUNC_INFO << "<!> m_algo isn't set";

    return suffix(m_algo);
}

QString DigestFile::suffix(QCryptographicHash::Algorithm algo)
{
    switch (algo) {
    case QCryptographicHash::Md5:
        return sl_digest_exts.at(0);
    case QCryptographicHash::Sha1:
        return sl_digest_exts.at(1);
    case QCryptographicHash::Sha256:
        return sl_digest_exts.at(2);
    case QCryptographicHash::Sha512:
        return sl_digest_exts.at(3);
    default:
        return {};
    }
}

QString DigestFile::suffix(int digestLength)
{
    return suffix(AlgoString::algoByStrLen(digestLength));
}

bool DigestFile::isDigestFile(const QString &filePath)
{
    return pathstr::hasExtension(filePath, sl_digest_exts);
}

bool DigestFile::make() const
{
    return make(m_mainFile, m_digest);
}

bool DigestFile::make(const QString &mainFile, const Digest &digest)
{
    if (mainFile.isEmpty() || !digest.isValid()) {
        qWarning() << Q_FUNC_INFO << "<!> Insufficient data";
        return false;
    }

    const QString sumFile = digestFilePath(mainFile, digest.algorithm());
    const QString strToWrite = tools::joinStrings(QString(digest),
                                                  pathstr::entryName(mainFile),
                                                  QStringLiteral(u" *"));

    QFile file(sumFile);

    return (file.open(QFile::WriteOnly)
            && file.write(strToWrite.toUtf8()));
}

Digest DigestFile::extractDigest(const QString &digestFile)
{
    QFile sumFile(digestFile);
    tools::openFile(sumFile, QFile::ReadOnly);

    // <!!!> TODO: should be rewritten with QByteArray
    const QString line = sumFile.readLine();

    static const QStringList l_seps { "  ", " *" };

    for (int it = 0; it <= l_seps.size(); ++it) {
        int cut = (it < l_seps.size()) ? line.indexOf(l_seps.at(it))
                                       : AlgoString::digestLength(AlgoString::strToAlgo(pathstr::suffix(digestFile)));

        if (cut > 0) {
            const Digest dig(line.left(cut).toUtf8());
            if (dig.isValid())
                return dig;
        }
    }

    throw Exception(ERR_NODATA, "Checksum not found");
}

QString DigestFile::digestFilePath(const QString &file, QCryptographicHash::Algorithm algo)
{
    return tools::joinStrings(file, suffix(algo), u'.');
}

QString DigestFile::digestFilePath(const QString &file, const int sum_len)
{
    return digestFilePath(file, AlgoString::algoByStrLen(sum_len));
}