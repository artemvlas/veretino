/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DIGEST_H
#define DIGEST_H

#include <QString>
#include <QCryptographicHash>

class DigestView
{
public:
    explicit DigestView(const QString &digest);

    bool isEmpty() const;

    // canBeChecksum
    bool isValid() const;
    bool isValid(QCryptographicHash::Algorithm algo) const;
    static bool isValid(const QString &digest);
    static bool isValid(const QString &digest, QCryptographicHash::Algorithm algo);

    bool verify(const QString &another) const;
    bool verify(const DigestView &another) const;
    static bool verify(const QString &first, const QString &second);

    QCryptographicHash::Algorithm algorithm() const;
    static QCryptographicHash::Algorithm algorithm(const QString &digest);

    const QString& toString() const { return *m_digest; }

    /*** Operators ***/
    explicit operator bool() const { return isValid(); }
    friend bool operator==(const DigestView& lhs, const DigestView& rhs) { return lhs.verify(rhs); }

private:
    const QString *m_digest;
}; // class DigestView


class Digest
{
public:
    explicit Digest(const QString &digest)
        : m_digest(digest) {}

    explicit Digest(QString&& digest) noexcept
        : m_digest(std::move(digest)) {}

    bool isEmpty() const {
        return m_digest.isEmpty();
    }

    bool isValid() const {
        return DigestView::isValid(m_digest);
    }
    bool isValid(QCryptographicHash::Algorithm algo) const {
        return DigestView::isValid(m_digest, algo);
    }

    bool verify(const QString &another) const {
        return DigestView::verify(m_digest, another);
    }
    bool verify(const Digest &another) const {
        return DigestView::verify(m_digest, another.toString());
    }

    QCryptographicHash::Algorithm algorithm() const {
        return DigestView::algorithm(m_digest);
    }

    const QString& toString() const {
        return m_digest;
    }

    /*** Operators ***/
    explicit operator bool() const { return isValid(); }
    friend bool operator==(const Digest& lhs, const Digest& rhs) { return lhs.verify(rhs); }

private:
    const QString m_digest;

}; // class Digest

#endif // DIGEST_H
