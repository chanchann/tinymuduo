// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/EventLoop.h>

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/Channel.h>
#include <muduo/net/Poller.h>
#include <muduo/net/SocketsOps.h>
#include <muduo/net/TimerQueue.h>

#include <boost/bind.hpp>

#include <signal.h>
#include <sys/eventfd.h>

using namespace muduo;
using namespace muduo::net;

namespace
{
// ��ǰ�߳�EventLoop����ָ��
// �ֲ߳̾��洢
__thread EventLoop* t_loopInThisThread = 0;  

const int kPollTimeMs = 10000;   //����Ĭ�ϵĳ�ʱʱ��

int createEventfd()
{
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0)
  {
    LOG_SYSERR << "Failed in eventfd";
    abort();
  }
  return evtfd;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe
{
 public:
  IgnoreSigPipe()
  {
    ::signal(SIGPIPE, SIG_IGN);   //����SIGPIP�ź�
    // LOG_TRACE << "Ignore SIGPIPE";
  }
};
#pragma GCC diagnostic error "-Wold-style-cast"

IgnoreSigPipe initObj;     // �൱����һ��ȫ�ֱ���
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
  return t_loopInThisThread;
}

EventLoop::EventLoop()
  : looping_(false),		//����ʱ��δѭ��
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    iteration_(0),
    threadId_(CurrentThread::tid()),  //��ʼ��Ϊ��ǰ�߳�
    poller_(Poller::newDefaultPoller(this)),
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_)),
    currentActiveChannel_(NULL)
{
  LOG_DEBUG << "EventLoop created " << this << " in thread " << threadId_;

  // �����ǰ�߳��Ѿ�������EventLoop������ֹ(LOG_FATAL)
  if (t_loopInThisThread)
  {
    LOG_FATAL << "Another EventLoop " << t_loopInThisThread
              << " exists in this thread " << threadId_;
  }
  else
  {
    t_loopInThisThread = this;
  }

  //����wakeupChannel�Ļص�����
  wakeupChannel_->setReadCallback(
      boost::bind(&EventLoop::handleRead, this));
  // we are always reading the wakeupfd
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
  LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
            << " destructs in thread " << CurrentThread::tid();
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  ::close(wakeupFd_);
  t_loopInThisThread = NULL;	//��ΪNULL����ʾ��ǰ�߳�����evenLoop	
}


// �¼�ѭ�����ú������ܿ��̵߳���
// ֻ���ڴ����ö�����߳��е���
void EventLoop::loop()
{
  //����δѭ�����ڵ�ǰ�߳���
  assert(!looping_);	
  assertInLoopThread();
  looping_ = true;
  quit_ = false;  // FIXME: what if someone calls quit() before loop() ?
  LOG_TRACE << "EventLoop " << this << " start looping";

  while (!quit_)
  {
    activeChannels_.clear();   //���ͨ�����
    //ͨ��poll���ػͨ���������activeChannel
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    ++iteration_;
	//��ӡ
    if (Logger::logLevel() <= Logger::TRACE)
    {
      printActiveChannels();
    }
    // TODO sort channel by priority
    eventHandling_ = true;
    for (ChannelList::iterator it = activeChannels_.begin();
        it != activeChannels_.end(); ++it)
    {
      currentActiveChannel_ = *it;  //���µ�ǰ���ڴ���ͨ��
      currentActiveChannel_->handleEvent(pollReturnTime_);
    }
    currentActiveChannel_ = NULL;   //����ǰ���ڴ���ͨ����ΪNULL
    eventHandling_ = false;    //���ڴ���ͨ��
    //�������ǰ�߳�Ҳ���Դ����������(��IO�����Ǻܶ�ʱ������������)
    doPendingFunctors();   
  }

  LOG_TRACE << "EventLoop " << this << " stop looping";
  looping_ = false;
}

//�ú������Կ��̵߳���,Ҳ���ǿ����������߳�����EvenLoop�˳�
void EventLoop::quit()
{
  quit_ = true;   //�̹߳���ģ����Կ���ֱ�Ӹ���
  // There is a chance that loop() just executes while(!quit_) and exits,
  // then EventLoop destructs, then we are accessing an invalid object.
  // Can be fixed using mutex_ in both places.
  //������ǵ�ǰ�߳��ڵ��øú�������ǰ�߳̿�����������poll��������
  //������Ҫ�ֶ�����wakeup����һ��IO�¼������̣߳�Ȼ���߳����������¼���ͻ���
  //�´�ѭ�����жϵ�quit_Ϊtrue���Ӷ��˳��¼�ѭ��
  if (!isInLoopThread())  
  {
    wakeup();   //������ǵ�ǰ�߳������wakeup���л��� 
  }
}


//����һ�ֱ��˼�룬�������߳���Ҫͨ������̵߳���Դ��ִ�������ʱ��
//������ֱ���������߳��з�����Դ���ú���
//�����ͻ������Դ�ľ�������Ҫ������֤�������������õ�ǰ�߳�
//Ϊ�����߳��ṩһ���ӿڣ������߳̽�Ҫִ�е�����������ӿڽ�����ǰ�߳�
//������ǰ�߳�ͳһ�����Լ�����Դ�������ü�����Ψһ��Ҫ�����ĵط�����
//ͨ���ӿ��������������������ط�������С��������

//��IO�߳���ִ��ĳ���ص��������ú������Կ��߳�ִ��
//�ɲ鿴���Գ���gaorongTests/Reactor_text05
void EventLoop::runInLoop(const Functor& cb)
{
  if (isInLoopThread())  //����ڵ�ǰIO�߳��е��ã���ͬ������cb,��ֱ�ӵ���
  {
    cb();
  }
  else
  {
    //����������߳��е��øú��������첽���ã���queueInLoop��ӵ������������
    queueInLoop(cb);
  }
}


//��������У��ȴ���ִ�У��ú������Կ��̵߳��ã��������߳̿��Ը���ǰ�߳��������
void EventLoop::queueInLoop(const Functor& cb)
{
  {	
  MutexLockGuard lock(mutex_);
  pendingFunctors_.push_back(cb);
  }

  //������ǵ�ǰ�߳�(����������poll)����Ҫ���� 
  //�����ǵ�ǰ�̵߳��������ڴ�������е�����(ʹ�ô����굱ǰ�����е�Ԫ�غ������ڽ�����һ�ִ�����Ϊ�����������������)��Ҫ����
  //ֻ�е�ǰIO�̵߳��¼��ص��е���queueInLoop�Ų���Ҫ����(��Ϊִ����handleEvent����Ȼִ��doPendingFunctor)
  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}

size_t EventLoop::queueSize() const
{
  MutexLockGuard lock(mutex_);
  return pendingFunctors_.size();
}

//���������������ǵ���addTimer������Ӷ�ʱ��
//�ɲ鿴���Գ���gaorongTests/Reactor_test04
TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
  return timerQueue_->addTimer(cb, time, 0.0);  //0.0������һ���ظ��Ķ�ʱ��
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerQueue_->addTimer(cb, time, interval);
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
// FIXME: remove duplication
void EventLoop::runInLoop(Functor&& cb)
{
  if (isInLoopThread())
  {
    cb();
  }
  else
  {
    queueInLoop(std::move(cb));
  }
}

void EventLoop::queueInLoop(Functor&& cb)
{
  {
  MutexLockGuard lock(mutex_);
  pendingFunctors_.push_back(std::move(cb));  // emplace_back
  }

  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}

TimerId EventLoop::runAt(const Timestamp& time, TimerCallback&& cb)
{
  return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback&& cb)
{
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, TimerCallback&& cb)
{
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerQueue_->addTimer(std::move(cb), time, interval);
}
#endif

void EventLoop::cancel(TimerId timerId)
{
  return timerQueue_->cancel(timerId);  //ֱ�ӵ���timerQueue��
}

void EventLoop::updateChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);  //����channle������EvenLoop������
  assertInLoopThread();     //����EvenLoop�ǵ�ǰ�߳�
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  if (eventHandling_)
  {
    assert(currentActiveChannel_ == channel ||
        std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
  }
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
  LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();
}

void EventLoop::wakeup()
{
  //evenfd�Ļ�����ֻ�а˸��ֽڣ�����ֻ��Ҫдһ��uint64_t����ֵ�Ϳ���
  uint64_t one = 1;
  ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

//wakeupChannel�Ļص�����
void EventLoop::handleRead()
{
  uint64_t one = 1;
  //�ڲ����õĻ���::read����
  ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

// 1. ���Ǽ򵥵����ٽ��������ε���Functor��
//���ǰѻص��б�swap��functors�У�����һ�����С���ٽ����ĳ���
//����ζ�Ų������������̵߳�queueInLoop()�������pendingFunctors_������һ���棬Ҳ��������
//������ΪFunctor�����ٴε���queueInLoop()�������pendingFunctors_��

//2. ����doPendingFunctors()���õ�Functor�����ٴε���queueInLoop(cb)�������pendingFunctors_����ʱ��
//queueInLoop()�ͱ���wakeup()������������cb���ܾͲ��ܼ�ʱ������

//3. muduoû�з���ִ��doPendingFunctors()ֱ��pendingFunctorsΪ�գ�
//��������ģ�����IO�߳̿���������ѭ�����޷�����IO�¼���
void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;  //�����ڴ��������flag

  {
  MutexLockGuard lock(mutex_);
  functors.swap(pendingFunctors_);  //����Ч�ʸ�
  }

  for (size_t i = 0; i < functors.size(); ++i)
  {
    functors[i]();
  }
  callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const
{
  for (ChannelList::const_iterator it = activeChannels_.begin();
      it != activeChannels_.end(); ++it)
  {
    const Channel* ch = *it;
    LOG_TRACE << "{" << ch->reventsToString() << "} ";
  }
}

