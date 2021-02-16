// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/Thread.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Exception.h>
#include <muduo/base/Logging.h>

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/weak_ptr.hpp>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>



namespace muduo
{


//C++�������ռ䶨��ı�������ͨ�ı���ûʲô����
//ֻ�����Ǽ���һ����ʿռ��ʶ����
namespace CurrentThread
{
	//��Currentthread.h��Ҳ����ͬ�������ռ�
	//__thread��gcc���õ��ֲ߳̾��洢��ʩ��__threadֻ������POD����
  __thread int t_cachedTid = 0; //�߳���ʵpid��tid�� ��ȡ������Ϊ�˷�ֹÿ��ϵͳЧ�ý���Ч��
  __thread char t_tidString[32];   //tid���ַ�����ʶ��ʽ
  __thread int t_tidStringLength = 6;
  __thread const char* t_threadName = "unknown";

  //����ʱ����
  const bool sameType = boost::is_same<int, pid_t>::value;
  BOOST_STATIC_ASSERT(sameType);
}

namespace detail
{

pid_t gettid()
{
  //����ϵͳ���û�ȡtid
  return static_cast<pid_t>(::syscall(SYS_gettid));
}

void afterFork()
{

  //�����̵߳����̵߳�����fork�󣬻Ὣ�ӽ��̵����߳���������Ϊmain 
  //���������ӽ����Ժ󴴽����Լ������߳�
  muduo::CurrentThread::t_cachedTid = 0;
  muduo::CurrentThread::t_threadName = "main";
  CurrentThread::tid();
  // no need to call pthread_atfork(NULL, NULL, &afterFork);
}

class ThreadNameInitializer
{
 public:
  ThreadNameInitializer()
  {
    muduo::CurrentThread::t_threadName = "main";
    CurrentThread::tid();
	
	//int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));
    //����forkʱ���ڲ������ӽ���ǰ�ڸ������л����prepare��
	//�ڲ������ӽ��̳ɹ��󣬸����̻����parent ���ӽ��̻����child
    pthread_atfork(NULL, NULL, &afterFork);
  }
};

//ȫ�ֱ�����ϵͳ������main����֮ǰ�͵��øú����
ThreadNameInitializer init;

//�����߳����ݵĽṹ��
struct ThreadData
{
  typedef muduo::Thread::ThreadFunc ThreadFunc;
  //��Ҫ���������ݳ�Ա
  ThreadFunc func_;
  string name_;
  //����һ�������ö��󣬷�ֹshare_ptr�����ڴ�й¶
  //share_ptr����ֱ�Ӹ�ֵ��weak_ptr����
  boost::weak_ptr<pid_t> wkTid_;

  ThreadData(const ThreadFunc& func,
             const string& name,
             const boost::shared_ptr<pid_t>& tid)
    : func_(func),
      name_(name),
      wkTid_(tid)
  { }

  void runInThread()
  {
    pid_t tid = muduo::CurrentThread::tid();

	//ͨ��boost::weak_ptr::lock�������shared_ptr����
    boost::shared_ptr<pid_t> ptid = wkTid_.lock();
    if (ptid)   
    {
      *ptid = tid;    //����ָ�Ķ�����и�ֵ
       ptid.reset();  //���ü�����1��ֹͣ��ָ��Ĺ���
    }

    muduo::CurrentThread::t_threadName = name_.empty() ? "muduoThread" : name_.c_str();
	//Ϊ�߳���������
	::prctl(PR_SET_NAME, muduo::CurrentThread::t_threadName);

	try
    {
      func_();
      muduo::CurrentThread::t_threadName = "finished"; //��ʶΪ����
    }
    catch (const Exception& ex)
    {
      muduo::CurrentThread::t_threadName = "crashed";
      fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
      fprintf(stderr, "reason: %s\n", ex.what());
      fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
      abort();
    }
    catch (const std::exception& ex)
    {
      muduo::CurrentThread::t_threadName = "crashed";
      fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
      fprintf(stderr, "reason: %s\n", ex.what());
      abort();
    }
    catch (...)
    {
      muduo::CurrentThread::t_threadName = "crashed";
      fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
      throw; // rethrow
    }
  }
};

//�������κ������ͨȫ�ֺ�������Ϊpthread_create�Ĳ���
void* startThread(void* obj)
{
  ThreadData* data = static_cast<ThreadData*>(obj);
  data->runInThread();
  delete data;
  return NULL;
}

}
}

using namespace muduo;

void CurrentThread::cacheTid()
{
  if (t_cachedTid == 0)
  {
    t_cachedTid = detail::gettid();
    t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
  }
}

bool CurrentThread::isMainThread()
{
	//���̵߳�pid����tid
  return tid() == ::getpid();
}

void CurrentThread::sleepUsec(int64_t usec)
{
  struct timespec ts = { 0, 0 };
  ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000);
  ::nanosleep(&ts, NULL);
}


AtomicInt32 Thread::numCreated_;

Thread::Thread(const ThreadFunc& func, const string& n)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(new pid_t(0)),
    func_(func),
    name_(n)
{
 //�߳��ഴ����ʱ������������ʵ�ֳ�ʼ��
  setDefaultName();
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
Thread::Thread(ThreadFunc&& func, const string& n)
  : started_(false),
    joined_(false),
    pthreadId_(0),
    tid_(new pid_t(0)),
    func_(std::move(func)),
    name_(n)
{
  setDefaultName();
}

#endif

Thread::~Thread()
{
  if (started_ && !joined_)
  {
   //�Զ���Դ����
    pthread_detach(pthreadId_);
  }
}

void Thread::setDefaultName()
{
  //�����̵߳ĸ�����1
  int num = numCreated_.incrementAndGet();
  if (name_.empty())
  {
    char buf[32];
	//���δ�������֣�Ĭ��δThread+�̺߳�
    snprintf(buf, sizeof buf, "Thread%d", num);
    name_ = buf;
  }
}

void Thread::start()
{
  assert(!started_);
  started_ = true;
  // FIXME: move(func_)

  //����һ��ThreadData�ṹ��
  detail::ThreadData* data = new detail::ThreadData(func_, name_, tid_);
  if (pthread_create(&pthreadId_, NULL, &detail::startThread, data))
  {
    //����ʧ�ܷ��ط�0��ִ�������֧
    started_ = false;
    delete data; // or no delete?
    LOG_SYSFATAL << "Failed in pthread_create";
  }
}

int Thread::join()
{
  assert(started_);
  assert(!joined_);
  joined_ = true;
  return pthread_join(pthreadId_, NULL);
}

