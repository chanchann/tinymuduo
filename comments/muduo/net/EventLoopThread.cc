// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/EventLoopThread.h>

#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;


EventLoopThread::EventLoopThread(const ThreadInitCallback& cb,
                                 const string& name)
  : loop_(NULL),
    exiting_(false),
    thread_(boost::bind(&EventLoopThread::threadFunc, this), name),
    mutex_(),
    cond_(mutex_),
    callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
  exiting_ = true;
  
  if (loop_ != NULL) // not 100% race-free, eg. threadFunc could be running callback_.
  {
    // still a tiny chance to call destructed object, if threadFunc exits just now.
    // but when EventLoopThread destructs, usually programming is exiting anyway.
    loop_->quit();  // �˳�IO�̣߳���IO�̵߳�loopѭ���˳����Ӷ��˳���IO�߳�
    thread_.join();
  }
}

EventLoop* EventLoopThread::startLoop()
{
  assert(!thread_.started());  //�����̻߳�û������
  thread_.start();		//�����̣߳����ûص�����threadFunc
  //����������ú��������һ���µ��̣߳����̼߳�������ִ�У����߳�ִ��threadFunc

  {
    MutexLockGuard lock(mutex_);
    while (loop_ == NULL)   //ͨ�����ַ�ʽ�ж��߳���EvenLoop�Ƿ񴴽����
    {
      cond_.wait();
    }
  }

  return loop_;
}

void EventLoopThread::threadFunc()
{
  EventLoop loop;  //����һ������

  if (callback_)  //����ThreadInitCallback
  {
    callback_(&loop);
  }

  {
    MutexLockGuard lock(mutex_);	
    // loop_ָ��ָ����һ��ջ�ϵĶ���threadFunc�����˳�֮�����ָ���ʧЧ��
    // threadFunc�����˳�������ζ���߳��˳��ˣ�EventLoopThread����Ҳ��û�д��ڵļ�ֵ�ˡ�
    // ���������ʲô�������

    //loop_ָ��ִ��ջ�ϵĶ���threadFunc�����˳�֮�����ָ���ʧЧ��
    //��ΪEventLoopThread�ڳ���ʼʱ�򴴽����ڳ����˳�ʱ��������EventLoopThreadһֱ������������������
    //����loop_һֱ�ڴ��ڼ���Ч���������˳�ʱ��loop_Ҳ�Ͳ���������
	loop_ = &loop;
    cond_.notify();
  }

  loop.loop();   //���߳���ִ��loopѭ��
  //assert(exiting_);
  loop_ = NULL;
}

