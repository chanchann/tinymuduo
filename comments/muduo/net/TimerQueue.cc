// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <muduo/net/TimerQueue.h>

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Timer.h>
#include <muduo/net/TimerId.h>

#include <boost/bind.hpp>

#include <sys/timerfd.h>

namespace muduo
{
namespace net
{
namespace detail
{

int createTimerfd()  //������ʱ��
{
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                 TFD_NONBLOCK | TFD_CLOEXEC);
  if (timerfd < 0)
  {
    LOG_SYSFATAL << "Failed in timerfd_create";
  }
  return timerfd;
}

//���㳬ʱʱ���뵱ǰʱ���ʱ���
struct timespec howMuchTimeFromNow(Timestamp when)
{
  int64_t microseconds = when.microSecondsSinceEpoch()
                         - Timestamp::now().microSecondsSinceEpoch();
  if (microseconds < 100)
  {
    microseconds = 100;
  }
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(
      microseconds / Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>(
      (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
  return ts;
}


//�����ʱ��������һֱ����
void readTimerfd(int timerfd, Timestamp now)
{
  uint64_t howmany;
  ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
  LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
  if (n != sizeof howmany)
  {
    LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
  }
}

//���ö�ʱ���ĳ�ʱʱ��
void resetTimerfd(int timerfd, Timestamp expiration)
{
  // wake up loop by timerfd_settime()
  struct itimerspec newValue;
  struct itimerspec oldValue;
  bzero(&newValue, sizeof newValue);
  bzero(&oldValue, sizeof oldValue);
  newValue.it_value = howMuchTimeFromNow(expiration);
  //ϵͳ���ô���һ����ʱ��
  int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
  if (ret)
  {
    LOG_SYSERR << "timerfd_settime()";
  }
}

}
}
}

using namespace muduo;
using namespace muduo::net;
using namespace muduo::net::detail;


//��Ҫ������������EvenLoop
TimerQueue::TimerQueue(EventLoop* loop)
  : loop_(loop),
    timerfd_(createTimerfd()),
    timerfdChannel_(loop, timerfd_),
    timers_(),
    callingExpiredTimers_(false)
{
  //�趨�ص�����ΪhandleRead
  timerfdChannel_.setReadCallback(
      boost::bind(&TimerQueue::handleRead, this));
  // we are always reading the timerfd, we disarm it with timerfd_settime.
  timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
  timerfdChannel_.disableAll();
  timerfdChannel_.remove();
  ::close(timerfd_);
  // do not remove channel, since we're in EventLoop::dtor();
  for (TimerList::iterator it = timers_.begin();
      it != timers_.end(); ++it)
  {
    delete it->second;
  }
}


//addTimer�Ǹ��̰߳�ȫ�첽���õģ���Ϊ�����е���Ӷ�ʱ����������IO�߳�������
//��IO�߳���Ӷ�ʱ�����Բ���Ҫ����
TimerId TimerQueue::addTimer(const TimerCallback& cb,
                             Timestamp when,
                             double interval)
{
  //����һ��timer
  Timer* timer = new Timer(cb, when, interval);
  loop_->runInLoop(
      boost::bind(&TimerQueue::addTimerInLoop, this, timer));
  return TimerId(timer, timer->sequence());
}

#ifdef __GXX_EXPERIMENTAL_CXX0X__
TimerId TimerQueue::addTimer(TimerCallback&& cb,
                             Timestamp when,
                             double interval)
{
  Timer* timer = new Timer(std::move(cb), when, interval);
  loop_->runInLoop(
      boost::bind(&TimerQueue::addTimerInLoop, this, timer));

  //�������ֱ��addTimerInLoop���ã���ô����̹߳�ͬ����
  //��ʱ�������ݽṹ����Ҫ�����ĵط��϶࣬�����ڳ����Ч����
  //����ǳ���Ϊʲô��Ҫ����runInLoop��������
  //��ʹ������������Լ�ִ�У��������еĲ�����������
  return TimerId(timer, timer->sequence());
}
#endif

void TimerQueue::cancel(TimerId timerId)
{
  loop_->runInLoop(
      boost::bind(&TimerQueue::cancelInLoop, this, timerId));
}


void TimerQueue::addTimerInLoop(Timer* timer)
{
  //������������IO�߳���
  loop_->assertInLoopThread();
  //����һ����ʱ�����п��ܻ�ʹ���絽�ڵĶ�ʱ�������ı�
  bool earliestChanged = insert(timer);

  if (earliestChanged)
  {
   //���ö�ʱ���ĳ�ʱʱ��
    resetTimerfd(timerfd_, timer->expiration());
  }
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
  loop_->assertInLoopThread();
  assert(timers_.size() == activeTimers_.size());
  ActiveTimer timer(timerId.timer_, timerId.sequence_);
  //���Ҹö�ʱ��
  ActiveTimerSet::iterator it = activeTimers_.find(timer);
  if (it != activeTimers_.end())
  {
    size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
    assert(n == 1); (void)n;
    delete it->first; // FIXME: no delete please  �������unique_ptr,����Ͳ���Ҫ�ֶ�ɾ����
    activeTimers_.erase(it);
  }
  else if (callingExpiredTimers_)   //�������handleRead����������ڶ�ʱ����
  {    
    // �Ѿ����ڣ��������ڵ��ûص������Ķ�ʱ��
    cancelingTimers_.insert(timer);  //ֱ�Ӽ��뵽cancle�б��У��ص���������������resetɾ����
  }
  assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::handleRead()
{
  loop_->assertInLoopThread();
  Timestamp now(Timestamp::now());
  readTimerfd(timerfd_, now);   //������¼�������һֱ����

  //��ȡ��ʱ��ǰ���еĶ�ʱ���б�(����ʱ��ʱ���б�)
  //��Ȼ���ǻ�ȡ�������糬ʱ�Ķ�ʱ���������п��ܶ����ʱ���ĳ�ʱʱ����һ����
  //����������Ҫ�����г�ʱ�Ķ�ʱ�����ҳ���
  std::vector<Entry> expired = getExpired(now);

  callingExpiredTimers_ = true;  //������ʱ��״̬��flag��
  cancelingTimers_.clear();   //����Ѿ�ȡ�����ģ�
  // safe to callback outside critical section
  for (std::vector<Entry>::iterator it = expired.begin();
      it != expired.end(); ++it)
  {
    it->second->run();		//����ص���ʱ��������
  }
  callingExpiredTimers_ = false;

  reset(expired, now);   //�������һ���Զ�ʱ������Ҫ����
}


// rvo �������ڶԷ��ص�������п������������Ϊ���������rvo�Ż�
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
  assert(timers_.size() == activeTimers_.size());
  std::vector<Entry> expired;  //����timer��vector
  //��now�����Entry��ע������UINTPTR_MAX�ǹ��⹹������ֵ
  Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX)); 

  // ���ص�һ��δ���ڵ�Timer�ĵ�����
  // lower_bound�ĺ����Ƿ��ص�һ��ֵ>=sentry��Ԫ�ص�iterator
  // ��*end >= sentry���Ӷ�end->first > now
  TimerList::iterator end = timers_.lower_bound(sentry);
  //����Ҫôû�ҵ���Ҫô�ҵ���ʱ����ڵ�ǰʱ��
  assert(end == timers_.end() || now < end->first);
  
  // �����ڵĶ�ʱ�����뵽expired��
  std::copy(timers_.begin(), end, back_inserter(expired));
  
  // ��timers_���Ƴ����ڵĶ�ʱ��
  timers_.erase(timers_.begin(), end);
  // ��activeTimers_���Ƴ����ڵĶ�ʱ��
  for (std::vector<Entry>::iterator it = expired.begin();
      it != expired.end(); ++it)
  {
    ActiveTimer timer(it->second, it->second->sequence());
    size_t n = activeTimers_.erase(timer);
    assert(n == 1); (void)n;
  }

  assert(timers_.size() == activeTimers_.size());
  return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
  Timestamp nextExpire;

  for (std::vector<Entry>::const_iterator it = expired.begin();
      it != expired.end(); ++it)
  {
    ActiveTimer timer(it->second, it->second->sequence());

    // ������ظ��Ķ�ʱ��������δȡ����ʱ�����������ö�ʱ��
    if (it->second->repeat()
        && cancelingTimers_.find(timer) == cancelingTimers_.end())
    {
      it->second->restart(now);
      insert(it->second);
    }
    else
    {
	  // һ���Զ�ʱ�������ѱ�ȡ���Ķ�ʱ���ǲ������õģ����ɾ���ö�ʱ��
      // FIXME move to a free list
      delete it->second; // FIXME: no delete please
    }
  }

  if (!timers_.empty())
  {
    nextExpire = timers_.begin()->second->expiration();
  }

  if (nextExpire.valid())
  {
    resetTimerfd(timerfd_, nextExpire);
  }
}

bool TimerQueue::insert(Timer* timer)
{
  loop_->assertInLoopThread();
  assert(timers_.size() == activeTimers_.size()); //��������set��Сһ��
  bool earliestChanged = false;         //���絽��ʱ���Ƿ�ı�
  Timestamp when = timer->expiration();
  TimerList::iterator it = timers_.begin(); //��Ϊ�����򼯺����Ե�һ������Ҫ���ڵ�
  if (it == timers_.end() || when < it->first)
  {
    //timers_Ϊ�գ�����whenС��timer_�����絽��ʱ��
    earliestChanged = true;
  }
  {
  	//���뵽timer��
    std::pair<TimerList::iterator, bool> result
      = timers_.insert(Entry(when, timer));
    assert(result.second); (void)result;
  }
  {
  	//���뵽activeTimers_��
    std::pair<ActiveTimerSet::iterator, bool> result
      = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
    assert(result.second); (void)result;
  }

  assert(timers_.size() == activeTimers_.size());
  return earliestChanged;
}

