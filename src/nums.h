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
    NumSize(int num, qint64 size) : number(num), total_size(size) {}
    void add(int num, qint64 size) { number += num; total_size += size; }
    void add(const NumSize &other) { add(other.number, other.total_size); }
    void addOne(qint64 size = 0) { ++number; total_size += size; }
    void subtract(int num, qint64 size) { number -= num; total_size -= size; }
    void subtract(const NumSize &other) { subtract(other.number, other.total_size); }
    void subtractOne(qint64 size = 0) { --number; total_size -= size; }
    void reset() { number = 0; total_size = 0; }
    NumSize& operator<<(qint64 size) { addOne(size); return *this; }
    NumSize& operator<<(const NumSize &other) { add(other); return *this; }
    NumSize& operator+=(const NumSize &other) { add(other); return *this; }
    NumSize& operator-=(const NumSize &other) { subtract(other); return *this; }
    NumSize& operator-=(qint64 size) { subtractOne(size); return *this; }
    NumSize& operator++() { ++number; return *this; } // prefix
    friend NumSize operator+(NumSize lhs, const NumSize& rhs) { lhs += rhs; return lhs; }
    friend NumSize operator-(NumSize lhs, const NumSize& rhs) { lhs -= rhs; return lhs; }
    friend bool operator==(const NumSize& lhs, const NumSize& rhs) { return (lhs.number == rhs.number) && (lhs.total_size == rhs.total_size); }
    friend bool operator!=(const NumSize& lhs, const NumSize& rhs) { return !(lhs == rhs); }
    friend bool operator< (const NumSize& lhs, const NumSize& rhs) { return (lhs.total_size < rhs.total_size) || (lhs.total_size == rhs.total_size && lhs.number < rhs.number); }
    friend bool operator> (const NumSize& lhs, const NumSize& rhs) { return (rhs < lhs); }
    friend bool operator<=(const NumSize& lhs, const NumSize& rhs) { return (lhs < rhs) || (lhs == rhs); }
    friend bool operator>=(const NumSize& lhs, const NumSize& rhs) { return (rhs < lhs) || (lhs == rhs); }
    explicit operator bool() const { return number > 0; }

    // values
    int number = 0;
    qint64 total_size = 0;
}; // struct NumSize

template <typename T = int> // parts of progress (int, qint64...)
struct Chunks {
    Chunks() {}
    Chunks(T total_number) : total(total_number) {}
    T remain() const { return total - done; }
    int percent() const { return total > 0 ? (done * 100) / total : 0; }
    void setTotal(T total_number) { total = total_number; done = 0; }
    bool decreaseTotal(T by_size) { if (remain() < by_size) return false; total -= by_size; return true; }
    bool isSet() const { return total > 0; }
    bool hasChunks() const { return done > 0; }
    void addChunks(T number) { done += number; }                              // same as P <<
    void addChunk() { ++done; }                                               // same as ++P
    void reset() { setTotal(0); }
    Chunks& operator<<(T done_number) { done += done_number; return *this; }
    Chunks& operator++() { ++done; return *this; }                            // prefix
    Chunks  operator++(int) { Chunks res(*this); ++(*this); return res; }     // postfix
    explicit operator bool() const { return total > 0; }                      // same as isSet()

    // values
    T total = 0;
    T done = 0;
}; // struct Chunks

#endif // NUMS_H
