#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H

#include <boost/noncopyable.hpp>
#include <boost/functional.hpp>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>

class EventLoop
{
  public:
  void print_info()
  {
      pid_t tid = static_cast<pid_t>(::syscall(SYS_thread_selfid));
      printf("The caller is in tid = %d\n", tid);
  }

  void print_self()
  {
      pid_t tid = static_cast<pid_t>(::syscall(SYS_thread_selfid));
      printf("in IO, tid = %d\n", tid);
  }
}
;

class EventLoopThread : boost::noncopyable
{
 public:
  EventLoopThread();
  ~EventLoopThread();
  EventLoop* startLoop1();	// 启动一个新线程，该新线程就成为了IO线程

 private:
  static void* startThread(void* arg);		// 新线程函数

  EventLoop* loop_;			// loop_指针指向一个EventLoop对象
  pthread_t main_tid;
};

#endif  // MUDUO_NET_EVENTLOOPTHREAD_H

