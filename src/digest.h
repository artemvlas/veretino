/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef DIGEST_H
#define DIGEST_H

#include <QCryptographicHash>

class DigestView
{
public:
    explicit DigestView(const QByteArray &digest);

    bool isEmpty() const;

    // canBeChecksum
    bool isValid() const;
    bool isValid(QCryptographicHash::Algorithm algo) const;
    static bool isValid(const QByteArray &digest);
    static bool isValid(const QByteArray &digest, QCryptographicHash::Algorithm algo);

    bool verify(const QByteArray &another) const;
    bool verify(const DigestView &another) const;
    static bool verify(const QByteArray &first, const QByteArray &second);

    QCryptographicHash::Algorithm algorithm() const;
    static QCryptographicHash::Algorithm algorithm(const QByteArray &digest);

    const QByteArray& digest() const { return *m_digest; }

    /*** Operators ***/
    explicit operator bool() const { return isValid(); }
    friend bool operator==(const DigestView& lhs, const DigestView& rhs) { return lhs.verify(rhs); }
    operator const QByteArray&() const { return *m_digest; }

private:
    const QByteArray *m_digest;
}; // class DigestView


class Digest
{
public:
    explicit Digest() {}

    explicit Digest(const QByteArray &digest)
        : m_digest(digest) {}

    explicit Digest(QByteArray&& digest) noexcept
        : m_digest(std::move(digest)) {}

    void set(QByteArray digest) {
        m_digest = std::move(digest);
    }

    bool isEmpty() const {
        return m_digest.isEmpty();
    }

    bool isValid() const {
        return DigestView::isValid(m_digest);
    }
    bool isValid(QCryptographicHash::Algorithm algo) const {
        return DigestView::isValid(m_digest, algo);
    }

    bool verify(const QByteArray &another) const {
        return DigestView::verify(m_digest, another);
    }
    bool verify(const Digest &another) const {
        return DigestView::verify(m_digest, another.digest());
    }

    QCryptographicHash::Algorithm algorithm() const {
        return DigestView::algorithm(m_digest);
    }

    const QByteArray& digest() const {
        return m_digest;
    }

    /*** Operators ***/
    explicit operator bool() const { return isValid(); }
    friend bool operator==(const Digest& lhs, const Digest& rhs) { return lhs.verify(rhs); }
    operator const QByteArray&() const { return m_digest; }

    // Digest& operator=(const QByteArray &digest) {
    //     m_digest = digest;
    //     return *this;
    // }

private:
    QByteArray m_digest;

}; // class Digest

#endif // DIGEST_H
