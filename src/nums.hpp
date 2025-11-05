/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef NUMS_HPP
#define NUMS_HPP

/* Stores the number and total size of some items. For example, files.
 * Provides the ability to perform basic arithmetic operations and compare values.
 */
struct NumSize {
    NumSize() {}
    NumSize(int num, long long size) : number(num), total_size(size) {}

    void add(int num, long long size) { number += num; total_size += size; }
    void add(const NumSize &other) { add(other.number, other.total_size); }
    void addOne(long long size = 0) { ++number; total_size += size; }
    void subtract(int num, long long size) { number -= num; total_size -= size; }
    void subtract(const NumSize &other) { subtract(other.number, other.total_size); }
    void subtractOne(long long size = 0) { --number; total_size -= size; }
    void reset() { number = 0; total_size = 0; }

    NumSize& operator<<(long long size) { addOne(size); return *this; }
    NumSize& operator<<(const NumSize &other) { add(other); return *this; }
    NumSize& operator+=(const NumSize &other) { add(other); return *this; }
    NumSize& operator-=(const NumSize &other) { subtract(other); return *this; }
    NumSize& operator-=(long long size) { subtractOne(size); return *this; }
    NumSize& operator++() { ++number; return *this; } // prefix
    friend NumSize operator+(NumSize lhs, const NumSize& rhs) { lhs += rhs; return lhs; }
    friend NumSize operator-(NumSize lhs, const NumSize& rhs) { lhs -= rhs; return lhs; }
    friend bool operator==(const NumSize& lhs, const NumSize& rhs) { return (lhs.number == rhs.number) && (lhs.total_size == rhs.total_size); }
    friend bool operator!=(const NumSize& lhs, const NumSize& rhs) { return !(lhs == rhs); }
    friend bool operator< (const NumSize& lhs, const NumSize& rhs) { return (lhs.total_size < rhs.total_size)
                                                                            || (lhs.total_size == rhs.total_size && lhs.number < rhs.number); }
    friend bool operator> (const NumSize& lhs, const NumSize& rhs) { return (rhs < lhs); }
    friend bool operator<=(const NumSize& lhs, const NumSize& rhs) { return (lhs < rhs) || (lhs == rhs); }
    friend bool operator>=(const NumSize& lhs, const NumSize& rhs) { return (rhs < lhs) || (lhs == rhs); }
    explicit operator bool() const { return number > 0; }

    /*** Values ***/
    int number = 0;
    long long total_size = 0;
}; // struct NumSize

/* Stores and operates progress values.
 * <T> is assumed to be an integer type (int, long long...)
 */
template <typename T = int>
struct Chunks {
    Chunks() {}
    Chunks(T total_number) : total(total_number) {}

    T remain() const { return total - done; }
    int percent() const { return total > 0 ? (done * 100) / total : 0; }
    void setTotal(T total_number) { total = total_number; done = 0; }
    bool decreaseTotal(T by_size) { if (remain() < by_size) return false; total -= by_size; return true; }
    bool isSet() const { return total > 0; }
    bool hasChunks() const { return done > 0; }
    void addChunks(T number) { done += number; } // same as P <<
    void addChunk() { ++done; }                  // same as ++P
    void reset() { setTotal(0); }

    Chunks& operator<<(T done_number) { done += done_number; return *this; }
    Chunks& operator++() { ++done; return *this; }                            // prefix
    Chunks  operator++(int) { Chunks res(*this); ++(*this); return res; }     // postfix
    explicit operator bool() const { return total > 0; }                      // same as isSet()

    /*** Values ***/
    T total = 0;
    T done = 0;
}; // struct Chunks

#endif // NUMS_HPP
