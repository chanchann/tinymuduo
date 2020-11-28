#ifndef MUDUO_BASE_COPYABLE_H
#define MUDUO_BASE_COPYABLE_H

namespace muduo {

/// A tag class emphasises the objects are copyable.
/// The empty base class optimization applies.
/// Any derived class of copyable should be a value type.

// 空基类，标识型，继承的都是值类型
// 值语义： 可以拷贝，拷贝之后，与原对象脱离关系
// 对象语义 ： 要么是不能拷贝，要么可以拷贝但拷贝之后与原对象仍然存下一定的关系，比如共享底层资源(要实现自己的拷贝构造函数)
// muduo中大部分对象都是不能拷贝的
class copyable {
protected:
    copyable() = default;
    ~copyable() = default;
};

}  // namespace muduo

#endif  // MUDUO_BASE_COPYABLE_H
