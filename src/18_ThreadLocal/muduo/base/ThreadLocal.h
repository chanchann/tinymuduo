// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCAL_H
#define MUDUO_BASE_THREADLOCAL_H

#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{

template<typename T>
class ThreadLocal : boost::noncopyable
{
 public:
  ThreadLocal()
  {
    pthread_key_create(&pkey_, &ThreadLocal::destructor);        // 创建一个Key，注册销毁TSD的函数。这个key是线程间共享，但指向的数据是TLS的
  }

  ~ThreadLocal()
  {
    pthread_key_delete(pkey_);        // 只是销毁Key
  }

  T& value()
  {
    T* perThreadValue = static_cast<T*>(pthread_getspecific(pkey_));      // 这个指针是数据指针，不是Key指针
    if (!perThreadValue) {
      T* newObj = new T();                       // 堆上数据
      pthread_setspecific(pkey_, newObj);       // 还没set数据，新构造一个并set到key
      perThreadValue = newObj;
    }
    return *perThreadValue;        // 这是不是返回局部对象的引用？不是。见readme。
  }

 private:

  static void destructor(void *x)          // 静态函数，销毁key指向的对象
  {
    T* obj = static_cast<T*>(x);
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    delete obj;
  }

 private:
  pthread_key_t pkey_;        // 指向的数据是堆上数据，通过回调函数释放
};

}
#endif
