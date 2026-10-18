/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DIGESTSTRING_H
#define DIGESTSTRING_H

#include <QString>
#include <QCryptographicHash>

class DigestStringView
{
public:
    explicit DigestStringView(const QString &digest);

    bool isEmpty() const;

    // canBeChecksum
    bool isValid() const;
    bool isValid(QCryptographicHash::Algorithm algo) const;
    static bool isValid(const QString &digest);
    static bool isValid(const QString &digest, QCryptographicHash::Algorithm algo);

    bool verify(const QString &another) const;
    bool verify(const DigestStringView &another) const;
    static bool verify(const QString &first, const QString &second);

    QCryptographicHash::Algorithm algorithm() const;
    static QCryptographicHash::Algorithm algorithm(const QString &digest);

    const QString& toString() const { return *m_digest; }

    /*** Operators ***/
    explicit operator bool() const { return isValid(); }
    friend bool operator==(const DigestStringView& lhs, const DigestStringView& rhs) { return lhs.verify(rhs); }

private:
    const QString *m_digest;
}; // class DigestStringView


class DigestString
{
public:
    explicit DigestString(const QString &digest)
        : m_digest(digest) {}

    explicit DigestString(QString&& digest) noexcept
        : m_digest(std::move(digest)) {}

    bool isEmpty() const {
        return m_digest.isEmpty();
    }

    bool isValid() const {
        return DigestStringView::isValid(m_digest);
    }
    bool isValid(QCryptographicHash::Algorithm algo) const {
        return DigestStringView::isValid(m_digest, algo);
    }

    bool verify(const QString &another) const {
        return DigestStringView::verify(m_digest, another);
    }
    bool verify(const DigestString &another) const {
        return DigestStringView::verify(m_digest, another.toString());
    }

    QCryptographicHash::Algorithm algorithm() const {
        return DigestStringView::algorithm(m_digest);
    }

    const QString& toString() const {
        return m_digest;
    }

    /*** Operators ***/
    explicit operator bool() const { return isValid(); }
    friend bool operator==(const DigestString& lhs, const DigestString& rhs) { return lhs.verify(rhs); }

private:
    const QString m_digest;

}; // class DigestString

#endif // DIGESTSTRING_H
