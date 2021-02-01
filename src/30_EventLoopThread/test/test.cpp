#include <stdio.h>
#include "EventLoopThread.cpp"

int main()
{
  printf("main(): pid = %d, tid = %d\n",
         getpid(), static_cast<pid_t>(::syscall(SYS_thread_selfid)));

  EventLoopThread loopThread;
  EventLoop* loop = loopThread.startLoop1();

  // loop指针在主线程，去调用子线程中EventLoop的成员函数。到底算哪个线程在调用？
  loop->print_info();   
  sleep(1);

  printf("exit main().\n");
  return 0;
}