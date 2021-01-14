#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <stdio.h>

using namespace muduo;

class Test
{
 public:
  Test(int numThreads)
    : latch_(1),
      threads_(numThreads)
  {
    for (int i = 0; i < numThreads; ++i)
    {
      char name[32];
      snprintf(name, sizeof name, "work thread %d", i);
      threads_.push_back(new muduo::Thread(
            boost::bind(&Test::threadFunc, this), muduo::string(name)));  // bind(&Test::threadFunc, this) -> void foo()
    }
    for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::start, _1));   // 经典写法
  }

  void run()
  {
    latch_.countDown();                 // --count
  }

  void joinAll()
  {
    for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::join, _1));
  }

 private:

  void threadFunc()
  {
    latch_.wait();                    // 子线程wait，等待主线程countdown发号施令
    printf("tid=%d, %s started\n",
           CurrentThread::tid(),
           CurrentThread::name());

    

    printf("tid=%d, %s stopped\n",
           CurrentThread::tid(),
           CurrentThread::name());
  }

  CountDownLatch latch_;
  boost::ptr_vector<Thread> threads_;
};

int main()
{
  printf("pid=%d, tid=%d\n", ::getpid(), CurrentThread::tid());
  Test t(3);
  sleep(3);
  printf("pid=%d, tid=%d %s running ...\n", ::getpid(), CurrentThread::tid(), CurrentThread::name());
  t.run();                // 主线程发号施令，让子线程运作
  t.joinAll();

  printf("number of created threads %d\n", Thread::numCreated());
}


