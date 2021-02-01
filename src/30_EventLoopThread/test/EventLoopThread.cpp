#include "EventLoopThread.h"
#include <boost/bind.hpp>
#include <functional>

EventLoopThread::EventLoopThread()
  : loop_(NULL),
    main_tid(0)
    {
      
    }

EventLoopThread::~EventLoopThread()
{
  pthread_join(main_tid, NULL);
}

EventLoop* EventLoopThread::startLoop1()
{
  pthread_create(&main_tid, NULL, &startThread, this);
  sleep(5); // 等新线程创建好对象
  return loop_;
}

void* EventLoopThread::startThread(void* arg)
{
  EventLoop loop;
  loop.print_self();
  EventLoopThread* tmp = static_cast<EventLoopThread*>(arg);
  tmp->loop_ = &loop;
}

