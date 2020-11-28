#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include "muduo/base/Atomic.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Types.h"

#include <functional>
#include <memory>
#include <pthread.h>

// 两个重要的类Thread和ThreadData

namespace muduo {

// thread类实现了对于线程的封装,内部提供了start(),join(),和一些返回线程状态信息的方法
class Thread : noncopyable {
public:
    typedef std::function<void ()> ThreadFunc;
    // 构造函数,调用setDefaultName,用于各种成员传入参数
    // name有一个默认值，是一个空的字符串类
    explicit Thread(ThreadFunc, const string& name = string());
    // FIXME: make it movable in C++11
    // 析构函数负责把线程detach让其自己销毁
    ~Thread();

    void start();
    int join(); // return pthread_join()

    // 返回内部成员线程是否启动,线程ID,线程名字
    bool started() const { return started_; }
    // pthread_t pthreadId() const { return pthreadId_; }
    pid_t tid() const { return tid_; }
    const string& name() const { return name_; }
    // 返回本进程创建的线程数目
    static int numCreated() { return numCreated_.get(); }

private:
    // 设置m_name
    void setDefaultName();

    bool       started_;  // 线程是否启动
    bool       joined_;   // 是否被join回收
    pthread_t  pthreadId_;  // 线程ID
    pid_t      tid_;     // 进程ID
    ThreadFunc func_;    // 线程函数 
    string     name_;    // 线程名字
    CountDownLatch latch_;  // 倒计时计数

    static AtomicInt32 numCreated_;  // 本进程创建的线程数量
};

}  // namespace muduo
#endif  // MUDUO_BASE_THREAD_H
