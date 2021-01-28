// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_SINGLETON_H
#define MUDUO_BASE_SINGLETON_H

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <stdlib.h> // atexit

namespace muduo
{

template<typename T>
class Singleton : boost::noncopyable
{
 public:
  static T& instance()          // 对外唯一获取实例T的接口，通过类名调用
  {
    pthread_once(&ponce_, &Singleton::init);        // 1.只执行init()一次 2.线程安全
    return *value_;
  }

 private:              // 构造函数、赋值运算等都声明为私有
  Singleton();          // 无法构造一个Singleton
  ~Singleton();

  static void init()
  {
    value_ = new T();
    ::atexit(destroy);         // 在生命周期结束时，调用destroy()
  }

  static void destroy()
  {
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];    // 用于检查T是否为完整类：char[-1]
    delete value_;         // 释放动态内存
  }

 private:
  static pthread_once_t ponce_;
  static T*             value_;
};

template<typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;      // 静态成员初始化

template<typename T>
T* Singleton<T>::value_ = NULL;

}
#endif

