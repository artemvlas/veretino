#include "algostring.h"
#include "pathstr.h"

const QStringList AlgoString::sl_digest_exts = {
    QStringLiteral(u"md5"),
    QStringLiteral(u"sha1"),
    QStringLiteral(u"sha256"),
    QStringLiteral(u"sha512")
};

AlgoString::AlgoString(QCryptographicHash::Algorithm algo)
    : m_algo(algo) {}

AlgoString::AlgoString(int digestLength)
{
    m_algo = algoByStrLen(digestLength);
}

QCryptographicHash::Algorithm AlgoString::algorithm() const
{
    return m_algo;
}

QString AlgoString::name() const
{
    return name(m_algo);
}

QString AlgoString::name(QCryptographicHash::Algorithm algo)
{
    switch (algo) {
    case QCryptographicHash::Md5:
        return QStringLiteral(u"MD5");
    case QCryptographicHash::Sha1:
        return QStringLiteral(u"SHA-1");
    case QCryptographicHash::Sha256:
        return QStringLiteral(u"SHA-256");
    case QCryptographicHash::Sha512:
        return QStringLiteral(u"SHA-512");
    default:
        return "Unknown";
    }
}

QString AlgoString::name(int digestLength)
{
    return name(algoByStrLen(digestLength));
}

QString AlgoString::suffix() const
{
    return suffix(m_algo);
}

QString AlgoString::suffix(QCryptographicHash::Algorithm algo)
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

QString AlgoString::suffix(int digestLength)
{
    return suffix(algoByStrLen(digestLength));
}

bool AlgoString::isDigestFile(const QString &filePath)
{
    return pathstr::hasExtension(filePath, sl_digest_exts);
}

int AlgoString::digestLength() const
{
    return digestLength(m_algo);
}

int AlgoString::digestLength(QCryptographicHash::Algorithm algo)
{
    switch (algo) {
    case QCryptographicHash::Md5:
        return 32;
    case QCryptographicHash::Sha1:
        return 40;
    case QCryptographicHash::Sha256:
        return 64;
    case QCryptographicHash::Sha512:
        return 128;
    default:
        return 0;
    }
}

QCryptographicHash::Algorithm AlgoString::algoByStrLen(int digestLength)
{
    switch (digestLength) {
    case 32:
        return QCryptographicHash::Md5;
    case 40:
        return QCryptographicHash::Sha1;
    case 64:
        return QCryptographicHash::Sha256;
    case 128:
        return QCryptographicHash::Sha512;
    default:
        return static_cast<QCryptographicHash::Algorithm>(0);
    }
}

QCryptographicHash::Algorithm AlgoString::strToAlgo(const QString &strAlgo)
{
    QList<int> digits;

    for (QChar ch : strAlgo) {
        if (ch.isDigit())
            digits.append(ch.digitValue());
    }

    switch (digitsToNum(digits)) {
    case 1:
        return QCryptographicHash::Sha1;
    case 256:
        return QCryptographicHash::Sha256;
    case 512:
        return QCryptographicHash::Sha512;
    case 5:
        return QCryptographicHash::Md5;
    default:
        return static_cast<QCryptographicHash::Algorithm>(0);
    }
}

int AlgoString::digitsToNum(const QList<int> &digits)
{
    int number = 0;

    for (int dgt : digits) {
        number = (number * 10) + dgt;
    }

    return number;
}
