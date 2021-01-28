// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCALSINGLETON_H
#define MUDUO_BASE_THREADLOCALSINGLETON_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>
#include<stdio.h>

namespace muduo
{

template<typename T>
class ThreadLocalSingleton : boost::noncopyable
{
 public:

  static T& instance()
  {
    if (!t_value_)
    {
      t_value_ = new T();
      deleter_.set(t_value_);
    }
    return *t_value_;
  }

  static T* pointer()
  {
    return t_value_;
  }

 private:

  static void destructor(void* obj)
  {
    // assert(obj == t_value_);      这里会报错：illegal hardware instruction。TODO：为什么？见readme
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    // delete t_value_;    这里随着子线程结束，t_value_被置为NULL，所以对它调用delete并不能正确执行析构函数
    t_value_ = static_cast<T*>(obj);      // 此时通过key关联，obj依然指向对象，所以可以通过它来正确释放内存
		delete t_value_;
    t_value_ = 0;
  }

  class Deleter
  {
   public:
    Deleter()
    {
      pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);   // 线程结束时调用destructor(void* obj)，参数为与key绑定的对象
    }

    ~Deleter()
    {
      pthread_key_delete(pkey_);       
    }

    void set(T* newObj)
    {
      assert(pthread_getspecific(pkey_) == NULL);
      pthread_setspecific(pkey_, newObj);
    }

    pthread_key_t pkey_;     // 同名不同值
  };
  
  static __thread T* t_value_;
  static Deleter deleter_;
};

template<typename T>
__thread T* ThreadLocalSingleton<T>::t_value_ = 0;       // 模版类静态成员，在特化后才会分配内存。
                                                        // __thread变量值只能初始化为编译器常量(值在编译器就可以确定)

template<typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;    // 使用typename来提示后面是一个模版类名

}
#endif
