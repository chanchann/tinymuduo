#ifndef MUDUO_BASE_EXCEPTION_H
#define MUDUO_BASE_EXCEPTION_H

#include "muduo/base/Types.h"
#include <exception>

namespace muduo {

class Exception : public std::exception {
public:
    Exception(string what);
    ~Exception() noexcept override = default;

    // default copy-ctor and operator= are okay.
    // 覆盖了父类的what函数,用于打印异常信息
    // noexcept是告诉编译器函数中不会发生异常,这有利于编译器对程序做更多的优化。
    // override:若函数用其修饰,其继承的父类中必须提供同名的虚函数, TODO: 为何需要?
    const char* what() const noexcept override {
        return message_.c_str();
    }
    // 覆盖了父类的stackTree函数,用于打印栈信息
    const char* stackTrace() const noexcept {
        return stack_.c_str();
    }

private:
    string message_;  // 异常信息
    string stack_;  // 异常的栈信息
};

}  // namespace muduo

#endif  // MUDUO_BASE_EXCEPTION_H
