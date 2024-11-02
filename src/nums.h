/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef NUMS_H
#define NUMS_H

#include <QtGlobal>

struct NumSize { // number and total size (for example, files)
    NumSize() {}
    NumSize(int num, qint64 size) : _num(num), _size(size) {}
    void add(int num, qint64 size) { _num += num; _size += size; }
    void add(const NumSize &other) { add(other._num, other._size); }
    void addOne(qint64 size = 0) { ++_num; _size += size; }
    void subtract(int num, qint64 size) { _num -= num; _size -= size; }
    void subtract(const NumSize &other) { subtract(other._num, other._size); }
    void subtractOne(qint64 size = 0) { --_num; _size -= size; }
    void reset() { _num = 0; _size = 0; }
    NumSize& operator<<(qint64 size) { addOne(size); return *this; }
    NumSize& operator<<(const NumSize &other) { add(other); return *this; }
    NumSize& operator+=(const NumSize &other) { add(other); return *this; }
    NumSize& operator-=(const NumSize &other) { subtract(other); return *this; }
    NumSize& operator-=(qint64 size) { subtractOne(size); return *this; }
    NumSize& operator++() { ++_num; return *this; } // prefix
    friend NumSize operator+(NumSize lhs, const NumSize& rhs) { lhs += rhs; return lhs; }
    friend NumSize operator-(NumSize lhs, const NumSize& rhs) { lhs -= rhs; return lhs; }
    friend bool operator==(const NumSize& lhs, const NumSize& rhs) { return (lhs._num == rhs._num) && (lhs._size == rhs._size); }
    friend bool operator!=(const NumSize& lhs, const NumSize& rhs) { return !(lhs == rhs); }
    friend bool operator< (const NumSize& lhs, const NumSize& rhs) { return (lhs._num < rhs._num) && (lhs._size <= rhs._size); }
    friend bool operator> (const NumSize& lhs, const NumSize& rhs) { return (rhs < lhs); }
    friend bool operator<=(const NumSize& lhs, const NumSize& rhs) { return (lhs < rhs) || (lhs == rhs); }
    friend bool operator>=(const NumSize& lhs, const NumSize& rhs) { return (rhs < lhs) || (lhs == rhs); }
    explicit operator bool() const { return _num > 0; }

    // values
    int _num = 0;
    qint64 _size = 0;
}; // struct NumSize

template <typename T = int> // parts of progress (int, qint64...)
struct Chunks {
    Chunks() {}
    Chunks(T total) : _total(total) {}
    T remain() const { return _total - _done; }
    int percent() const { return (_done * 100) / _total; }
    void setTotal(T total) { _total = total; _done = 0; }
    bool decreaseTotal(T by_size) { if (remain() < by_size) return false; _total -= by_size; return true; }
    bool hasSet() const { return _total > 0; }
    bool hasChunks() const { return _done > 0; }
    void addChunks(T number) { _done += number; } // same as P <<
    void addChunk() { ++_done; } // same as ++P
    void reset() { setTotal(0); }
    Chunks& operator<<(T done_num) { _done += done_num; return *this; }
    Chunks& operator++() { ++_done; return *this; } // prefix
    Chunks  operator++(int) { Chunks _res(*this); ++(*this); return _res; } // postfix
    explicit operator bool() const { return _total > 0; } // same as hasSet()

    // values
    T _total = 0;
    T _done = 0;
}; // struct Chunks

#endif // NUMS_H
