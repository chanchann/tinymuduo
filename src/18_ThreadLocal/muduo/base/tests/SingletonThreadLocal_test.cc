#include <muduo/base/Singleton.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/ThreadLocal.h>
#include <muduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <stdio.h>

class Test : boost::noncopyable
{
 public:
  Test()
  {
    printf("tid=%d, constructing %p\n", muduo::CurrentThread::tid(), this);
  }

  ~Test()
  {
    printf("tid=%d, destructing %p %s\n", muduo::CurrentThread::tid(), this, name_.c_str());
  }

  const std::string& name() const { return name_; }
  void setName(const std::string& n) { name_ = n; }

 private:
  std::string name_;
};

/* 这里就体现了Singleton类里讲的要点了：
  1. Test并不是单例类，我们可以构建新的Test对象；
  2. ThreadLocal<Test>也不是单例类，我们可以像ThreadLocal_test.cc里面一样，构造多个对象；
  3. Singleton<muduo::ThreadLocal<Test>>才是单例类。我们只能通过Singleton::instance()来获取唯一的对象
  4. 该唯一的对象是ThreadLocal<Test>类型，所以我们可以在每个线程里，通过这唯一的对象构建各自线程独立的Test对象。
  */
#define STL muduo::Singleton<muduo::ThreadLocal<Test>>::instance().value()

void print()
{
  printf("tid=%d, %p name=%s\n",
         muduo::CurrentThread::tid(),
         &STL,
         STL.name().c_str());
}

void threadFunc(const char* changeTo)
{
  print();
  STL.setName(changeTo);
  sleep(1);
  print();
}

int main()
{
  STL.setName("main one");       // 主线程有自己的Test对象了
  muduo::Thread t1(boost::bind(threadFunc, "thread1"));   
  muduo::Thread t2(boost::bind(threadFunc, "thread2"));       // 这些线程只是有key，并没有构建对象
  t1.start();
  t2.start();               // 在threadFunc()中构造了各自的对象
  t1.join();
  print();
  t2.join();
  pthread_exit(0);
}
