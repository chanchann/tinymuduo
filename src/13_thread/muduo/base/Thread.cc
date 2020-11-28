#include "muduo/base/Thread.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Exception.h"
#include "muduo/base/Logging.h"

#include <type_traits>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo {
namespace detail {
// 得到唯一的线程ID
pid_t gettid() {
    return static_cast<pid_t>(::syscall(SYS_gettid));
}
// currentthread中以前定义了四个__thread变量,用来描述线程信息,在这里进行设置
void afterFork() {
    muduo::CurrentThread::t_cachedTid = 0;
    muduo::CurrentThread::t_threadName = "main";
    CurrentThread::tid();
    // no need to call pthread_atfork(NULL, NULL, &afterFork);
}
// 线程初始化函数,负责初始化线程信息
class ThreadNameInitializer {
public:
    ThreadNameInitializer() {
        muduo::CurrentThread::t_threadName = "main";
        CurrentThread::tid();
        pthread_atfork(NULL, NULL, &afterFork);
    }
};
// 全局线程初始化对象,完成currentthread中线程全局变量的设置
ThreadNameInitializer init;
//线程数据类
// ThreadData类封装了线程数据信息,包括线程名字,ID,线程函数,倒计时计数信息
// 这个类主要用于 pthread_create()函数中作为线程传参而使用的.
struct ThreadData {
    typedef muduo::Thread::ThreadFunc ThreadFunc;
    ThreadFunc func_;  // 线程函数
    string name_;      // 线程名字
    pid_t* tid_;       // 线程ID
    CountDownLatch* latch_;  // 倒计时计数

    ThreadData(ThreadFunc func,
              const string& name,
              pid_t* tid,
              CountDownLatch* latch)
      : func_(std::move(func)),
        name_(name),
        tid_(tid),
        latch_(latch)
    { }
    // 运行线程
    void runInThread() {
        // 设置类成员变量
        *tid_ = muduo::CurrentThread::tid();
        tid_ = NULL;
        latch_->countDown();
        latch_ = NULL;
        // 设置线程名字
        muduo::CurrentThread::t_threadName = name_.empty() ? "muduoThread" : name_.c_str();
        ::prctl(PR_SET_NAME, muduo::CurrentThread::t_threadName);
        try {
            func_(); // 具体的运行函数
            muduo::CurrentThread::t_threadName = "finished"; // 执行结束
        } catch (const Exception& ex) {
            muduo::CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
            abort();
        } catch (const std::exception& ex) {
            muduo::CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
            fprintf(stderr, "reason: %s\n", ex.what());
            abort();
        } catch (...) {
            muduo::CurrentThread::t_threadName = "crashed";
            fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
            throw; // rethrow
        }
    }
};
// 从属于detail命名空间,使用ThreadData类创建并且启动线程
void* startThread(void* obj) {
    ThreadData* data = static_cast<ThreadData*>(obj);
    data->runInThread();
    delete data;
    return NULL;
}

}  // namespace detail

// 之前currentthread中没有定义此方法,因此在这里进行定义
void CurrentThread::cacheTid() {
    //设置 __thread修饰的 currentthread::线程ID,线程名字长度
    if (t_cachedTid == 0) {
        t_cachedTid = detail::gettid();
        t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
}
//判断是否是主线程,进程ID是否是线程ID
bool CurrentThread::isMainThread() {
    return tid() == ::getpid();
}
// 线程挂起一段时间
void CurrentThread::sleepUsec(int64_t usec) {
    struct timespec ts = { 0, 0 };
    ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
    ::nanosleep(&ts, NULL); // 挂起当前线程
}

//thread类中的static 变量,表示当前进程创建的线程数目
AtomicInt32 Thread::numCreated_;

//构造函数,初始化线程信息,是否启动,回收,线程ID,进程ID,执行函数,线程名字,倒计时计数
Thread::Thread(ThreadFunc func, const string& n)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(0),
    func_(std::move(func)),
    name_(n),
    latch_(1) {
  setDefaultName();  // 构造时确定名字
}

// 析构时把线程detach,不用父进程回收子线程
Thread::~Thread() {
    if (started_ && !joined_) {
        pthread_detach(pthreadId_);
    }
}

// 设置线程名字,格式为 "Thread%d"
void Thread::setDefaultName() {
    // 当前进程的线程数目++
    int num = numCreated_.incrementAndGet();
    if (name_.empty()) {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}
// 启动,利用pthread_create()创建一个线程去执行detail::startThread函数
// 把ThreadData指针作为参数传入,在startThread函数中执行ThreadData::runInThread
// 完成具体的线程函数调用
void Thread::start() {
    assert(!started_);
    started_ = true;
    // FIXME: move(func_)
    detail::ThreadData* data = new detail::ThreadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthreadId_, NULL, &detail::startThread, data)) {
        // 线程创建失败
        started_ = false;
        delete data; // or no delete?
        LOG_SYSFATAL << "Failed in pthread_create";
    } else {
        // 线程创建成功
        latch_.wait();
        assert(tid_ > 0);
    }
}
// 回收子线程
int Thread::join() {
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}

}  // namespace muduo
