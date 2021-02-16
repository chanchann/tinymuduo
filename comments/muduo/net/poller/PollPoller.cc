// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/poller/PollPoller.h>

#include <muduo/base/Logging.h>
#include <muduo/base/Types.h>
#include <muduo/net/Channel.h>

#include <assert.h>
#include <errno.h>
#include <poll.h>

using namespace muduo;
using namespace muduo::net;

PollPoller::PollPoller(EventLoop* loop)
  : Poller(loop)
{
}

PollPoller::~PollPoller()
{
}

Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
  // XXX pollfds_ shouldn't change
  //����timeoutMs��������poll�ĳ�ʱʱ�䣬�����ʱ��δ�յ���Ϣ���Ӧnothing happend
  int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
  int savedErrno = errno;
  Timestamp now(Timestamp::now());
  if (numEvents > 0)
  {
    LOG_TRACE << numEvents << " events happended";
    fillActiveChannels(numEvents, activeChannels);
  }
  else if (numEvents == 0)
  {
    LOG_TRACE << " nothing happended";
  }
  else
  {
    if (savedErrno != EINTR)
    {
      errno = savedErrno;
      LOG_SYSERR << "PollPoller::poll()";
    }
  }
  return now;
}

//numEvents��ʾ���ص��¼�����
void PollPoller::fillActiveChannels(int numEvents,
                                    ChannelList* activeChannels) const
{
  for (PollFdList::const_iterator pfd = pollfds_.begin();
      pfd != pollfds_.end() && numEvents > 0; ++pfd)
  {
    if (pfd->revents > 0)
    {
      --numEvents;  //ÿ����һ����--
      //����fd�ҵ���Ӧ��channel������һЩ����
      ChannelMap::const_iterator ch = channels_.find(pfd->fd);
      assert(ch != channels_.end());
      Channel* channel = ch->second;
      assert(channel->fd() == pfd->fd);

	  //��channle���¼���������
      channel->set_revents(pfd->revents);
      // pfd->revents = 0;
      //��ӵ��ͨ��������
      activeChannels->push_back(channel);
    }
  }
}

//ע��͸��¹�ע���¼�
void PollPoller::updateChannel(Channel* channel)
{
  Poller::assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
  if (channel->index() < 0)
  {
    
    // a new one, add to pollfds_
	//���indexС����,˵���Ǹ��µ�ͨ����Channle��ctor���ʼ��Ϊ-1
    //�µ�channel�����Ҳ���
    assert(channels_.find(channel->fd()) == channels_.end());

 	//����poll��Ҫ�Ľṹ�岢��ӵ�������
    struct pollfd pfd;
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    pollfds_.push_back(pfd);
	
    int idx = static_cast<int>(pollfds_.size())-1;
    channel->set_index(idx);     //����index
    channels_[pfd.fd] = channel;   //��ӵ�map��
  }
  else
  {
    // update existing one  ����һ���Դ��ڵ�channel

	//���ȶ���
    assert(channels_.find( channel->fd()) != channels_.end() );
    assert(channels_[channel->fd()] == channel);
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));

    struct pollfd& pfd = pollfds_[idx];
	//�����Ǹ���֧Ϊʲô��-fd-1? �����뿴����
    assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd()-1);
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;

	//��һ��channel��ʱ����Ϊ�����ĸ��¼���������Poller���Ƴ���channel
    if (channel->isNoneEvent())
    {
      // ignore this pollfd
      //�������ֱ������Ϊ-1�����ó�����������Ϊ��removeChannel�Ż�
	  //��ֹfdΪ0�����Լ�ȥ1
	  //��fdΪ��ֵʱ������poll�᷵��POLLNVAL��
	  pfd.fd = -channel->fd()-1;
    }
  }
}

void PollPoller::removeChannel(Channel* channel)
{
  Poller::assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd();
  assert(channels_.find(channel->fd()) != channels_.end());
  assert(channels_[channel->fd()] == channel);

  //�ڵ���removeChannelǰ�����벻�ڹ�ע�¼�������updateChannel���¼���ΪNonEvenr
  //���⻹�ڹ�ע�¼���ʱ������Ƴ���
  assert(channel->isNoneEvent());


  int idx = channel->index();
  assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
  const struct pollfd& pfd = pollfds_[idx]; (void)pfd;
  assert(pfd.fd == -channel->fd()-1 && pfd.events == channel->events());
  size_t n = channels_.erase(channel->fd());
  assert(n == 1); (void)n;   //��key�Ƴ�ʱ����ֵΪ1
  
  if (implicit_cast<size_t>(idx) == pollfds_.size()-1)
  {
    pollfds_.pop_back();   //���idx�������������һ����ֱ��pop
  }
  else
  {
    //�����Ƴ����㷨���Ӷ�Ϊ-1������ɾ����Ԫ�������һ��Ԫ�ؽ�����pop_back
    int channelAtEnd = pollfds_.back().fd;
    iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);

	//���channelAtEndΪ��ֵ������Ϊ�������ʵ��fd,����-(-4)-1=3,��ʵ��fdΪ3
    if (channelAtEnd < 0)
    {
      channelAtEnd = -channelAtEnd-1;
    }
    channels_[channelAtEnd]->set_index(idx);   //����fd����channel��idx
    pollfds_.pop_back();
  }
}

