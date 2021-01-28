#include <muduo/base/ThreadLocal.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Thread.h>

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

/* 这里执行完毕后，之后所有线程都可以访问这两个对象（key），
  但是没有调用过muduo::ThreadLocal<Test>::value()的，就不会有自己的Test对象
*/
muduo::ThreadLocal<Test> testObj1; 
muduo::ThreadLocal<Test> testObj2;

void print()
{
  printf("tid=%d, obj1 %p name=%s\n",
         muduo::CurrentThread::tid(),
	 &testObj1.value(),
         testObj1.value().name().c_str());
  printf("tid=%d, obj2 %p name=%s\n",
         muduo::CurrentThread::tid(),
	 &testObj2.value(),
         testObj2.value().name().c_str());
}

void threadFunc()
{
  print();
  testObj1.value().setName("changed 1");        
  testObj2.value().setName("changed 42");
  print();
}

int main()
{
  testObj1.value().setName("main one");     // 主线程有自己的Test1对象了，名字叫main one
  print();
  muduo::Thread t1(threadFunc);        
  t1.start();                   // 子线程调用threadFunc(),有自己的两个Test对象了，名字分别叫1和42
  t1.join();
  testObj2.value().setName("main two");     // 主线程有自己的Test2对象了，名字叫main two
  print();

  pthread_exit(0);
}
