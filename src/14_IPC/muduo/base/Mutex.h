// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_MUTEX_H
#define MUDUO_BASE_MUTEX_H

#include <muduo/base/CurrentThread.h>
#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace muduo
{

class MutexLock : boost::noncopyable    // 不可拷贝：很好理解，要一把新锁，肯定是重新构造一个
{
 public:
  MutexLock()
    : holder_(0)
  {
    int ret = pthread_mutex_init(&mutex_, NULL);
    assert(ret == 0); (void) ret;
  }

  ~MutexLock()
  {
    assert(holder_ == 0);     // 断言本锁没有被任何线程占有，才能进行析构
    int ret = pthread_mutex_destroy(&mutex_);
    assert(ret == 0); (void) ret;
  }

  bool isLockedByThisThread()
  {
    return holder_ == CurrentThread::tid();
  }

  void assertLocked()
  {
    assert(isLockedByThisThread());
  }

  // internal usage

  void lock()
  {
    pthread_mutex_lock(&mutex_);
    holder_ = CurrentThread::tid();
  }

  void unlock()
  {
    holder_ = 0;
    pthread_mutex_unlock(&mutex_);
  }

  pthread_mutex_t* getPthreadMutex() /* non-const */
  {
    return &mutex_;
  }

 private:

  pthread_mutex_t mutex_;
  pid_t holder_;        // 标识占有锁的线程
};

class MutexLockGuard : boost::noncopyable      // RAII方式封装：在其构造时获取资源，在对象生命期控制对资源的访问使之始终保持有效，最后在析构的时候释放资源。
{
 public:
  explicit MutexLockGuard(MutexLock& mutex)
    : mutex_(mutex)
  {
    mutex_.lock();
  }

  ~MutexLockGuard()
  {
    mutex_.unlock();          // 析构时解锁，防止因忘记解锁而造成死锁（并没有销毁锁）
  }

 private:

  MutexLock& mutex_;        // 1. 不能直接在构造函数里初始化，必须用到初始化列表，且形参也必须是引用类型。
                            // 2. 凡是有引用类型的成员变量的类，不能有缺省构造函数。原因是引用类型的成员变量必须在类构造时进行初始化。
};

}

// Prevent misuse like:
// MutexLockGuard(mutex_);
// A tempory object doesn't hold the lock for long!
#define MutexLockGuard(x) error "Missing guard object name" // 这样的临时量使用锁无意义

#endif  // MUDUO_BASE_MUTEX_H
