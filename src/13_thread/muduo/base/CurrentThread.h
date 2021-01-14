// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

namespace muduo
{
namespace CurrentThread
{
  // internal
  /* extern: 声明一个全局变量，该全局变量的定义在其他地方；在这里声明过后可以直接使用；
   *__thread： 该标识符将全局变量从线程间共享，缩小范围到线程内全局可见（每个线程有自己单独的一份）
  */
  extern __thread int t_cachedTid;           // 线程真实pid(tid)的缓存，不用每次去SYSCALL(gettid())
  extern __thread char t_tidString[32];      // tid的字符串表示形式
  extern __thread const char* t_threadName;  // 线程的名字
  void cacheTid();

  inline int tid()
  {
    if (t_cachedTid == 0)
    {
      cacheTid();
    }
    return t_cachedTid;
  }

  inline const char* tidString() // for logging
  {
    return t_tidString;
  }

  inline const char* name()
  {
    return t_threadName;
  }

  bool isMainThread();
}
}

#endif
