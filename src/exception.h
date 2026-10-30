/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stdexcept>

class Exception : public std::runtime_error {
public:
    int errorCode;
    Exception(int code, const std::string& msg = std::string())
        : std::runtime_error(msg), errorCode(code) {}
}; // class Exception

#endif // EXCEPTION_H
