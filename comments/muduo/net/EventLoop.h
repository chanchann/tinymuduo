// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <vector>

#include <boost/any.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include <muduo/base/Mutex.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/TimerId.h>

namespace muduo
{
namespace net
{

class Channel;
class Poller;
class TimerQueue;

///
/// Reactor, at most one per thread.  һ���߳����һ��ractor
///
/// This is an interface class, so don't expose too much details.

//�ӿ���
class EventLoop : boost::noncopyable
{
 public:
  typedef boost::function<void()> Functor;   //�����ض���

  EventLoop();
  ~EventLoop();  // force out-line dtor, for scoped_ptr members.

  ///
  /// Loops forever.
  ///
  /// Must be called in the same thread as creation of the object.
  /// 
  void loop();

  /// Quits loop.
  ///
  /// This is not 100% thread safe, if you call through a raw pointer,
  /// better to call through shared_ptr<EventLoop> for 100% safety.
  void quit();

  ///
  /// Time when poll returns, usually means data arrival.
  ///
  Timestamp pollReturnTime() const { return pollReturnTime_; }

  int64_t iteration() const { return iteration_; }

  /// Runs callback immediately in the loop thread.
  /// It wakes up the loop, and run the cb.
  /// If in the same loop thread, cb is run within the function.
  /// Safe to call from other threads.
  // ��ϸ�Ķ�����
  void runInLoop(const Functor& cb);
  /// Queues callback in the loop thread.
  /// Runs after finish pooling.
  /// Safe to call from other threads.
  void queueInLoop(const Functor& cb);

  size_t queueSize() const;

#ifdef __GXX_EXPERIMENTAL_CXX0X__
  void runInLoop(Functor&& cb);
  void queueInLoop(Functor&& cb);
#endif

  // timers

  ///
  /// Runs callback at 'time'.
  /// Safe to call from other threads.
  ///
  //��ĳһʱ�����ж�ʱ���ص�����
  TimerId runAt(const Timestamp& time, const TimerCallback& cb);
  ///
  /// Runs callback after @c delay seconds.
  /// Safe to call from other threads.
  ///
  //��һ��ʱ�����ж�ʱ���ص�����
  TimerId runAfter(double delay, const TimerCallback& cb);
  ///
  /// Runs callback every @c interval seconds.
  /// Safe to call from other threads.
  ///
  //ÿ��һ��ʱ�����ж�ʱ���ص�����
  TimerId runEvery(double interval, const TimerCallback& cb);
  ///
  /// Cancels the timer.
  /// Safe to call from other threads.
  ///
  void cancel(TimerId timerId);

#ifdef __GXX_EXPERIMENTAL_CXX0X__
  TimerId runAt(const Timestamp& time, TimerCallback&& cb);
  TimerId runAfter(double delay, TimerCallback&& cb);
  TimerId runEvery(double interval, TimerCallback&& cb);
#endif

  // internal usage
  void wakeup();
  void updateChannel(Channel* channel);  //��poller�����ӻ����ͨ��
  void removeChannel(Channel* channel);  //��poller���Ƴ�ͨ��
  bool hasChannel(Channel* channel);

  // pid_t threadId() const { return threadId_; }
  //�������ڵ�ǰ�߳�
  void assertInLoopThread()
  {
    if (!isInLoopThread())
    {
      abortNotInLoopThread();
    }
  }
  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }
  // bool callingPendingFunctors() const { return callingPendingFunctors_; }
  bool eventHandling() const { return eventHandling_; }

  void setContext(const boost::any& context)
  { context_ = context; }

  const boost::any& getContext() const
  { return context_; }

  boost::any* getMutableContext()
  { return &context_; }

  static EventLoop* getEventLoopOfCurrentThread();

 private:
  void abortNotInLoopThread();
  void handleRead();  // waked up
  void doPendingFunctors();

  void printActiveChannels() const; // DEBUG

  typedef std::vector<Channel*> ChannelList;


  //��linux��bool���ͱ�������ԭ�Ӳ��������β�һ��!���Բ���Ҫ����
  bool looping_; /* atomic */	//�Ƿ���ѭ��
  bool quit_; /* atomic and shared between threads, okay on x86, I guess. */
  bool eventHandling_; /* atomic */  //�Ƿ����¼�����״̬
  bool callingPendingFunctors_; /* atomic */
  int64_t iteration_;
  const pid_t threadId_;	// ��ǰ���������߳�ID
  Timestamp pollReturnTime_;  //����poll���������ص�ʱ���
  boost::scoped_ptr<Poller> poller_;    //ֻ��ָ����󣬹���poller
  boost::scoped_ptr<TimerQueue> timerQueue_;   //����ʱ����TimeQueue
  int wakeupFd_;   //����evenfd,�μ�man(2)
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  //ֻ������channel����Ϲ�ϵ���������wakeupChannel����������
  boost::scoped_ptr<Channel> wakeupChannel_; //��ͨ��������poller_������
  boost::any context_;

  // scratch variables
  //channelList��EvenLoop�ǾۺϹ�ϵ����������ChannelList��Ԫ�ص���������
  ChannelList activeChannels_;      //��ǰ���صĻͨ������
  Channel* currentActiveChannel_;   //��ǰ���ڴ���Ļͨ��

  mutable MutexLock mutex_;      //pendingFunctors����
  //һ���������
  std::vector<Functor> pendingFunctors_; // @GuardedBy mutex_
};

}
}
#endif  // MUDUO_NET_EVENTLOOP_H
