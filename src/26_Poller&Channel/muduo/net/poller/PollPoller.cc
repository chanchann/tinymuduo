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

Timestamp PollPoller::poll(int timeoutMs, ChannelList* activeChannels)    // 调用poll()，返回事件返回的时间
{
  // XXX pollfds_ shouldn't change
  int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
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
    LOG_SYSERR << "PollPoller::poll()";
  }
  return now;
}

void PollPoller::fillActiveChannels(int numEvents,
                                    ChannelList* activeChannels) const  // 提取有事件发生的pfd，const说明只读
{
  for (PollFdList::const_iterator pfd = pollfds_.begin();
      pfd != pollfds_.end() && numEvents > 0; ++pfd)
  {
    if (pfd->revents > 0)
    {
      --numEvents;
      ChannelMap::const_iterator ch = channels_.find(pfd->fd);
      assert(ch != channels_.end());    // 断言能够找到对应的channel，否则是谁放进去的
      Channel* channel = ch->second;
      assert(channel->fd() == pfd->fd);    // 断言通过fd找到的channel，包含的正是此fd，确定其对应关系
      channel->set_revents(pfd->revents);   // 把返回的事件设置到channel里
      // pfd->revents = 0;
      activeChannels->push_back(channel);   // 这里只能push_back指针，对象是noncopyable的
    }
  }
}

void PollPoller::updateChannel(Channel* channel)
{
  Poller::assertInLoopThread();  // 不能跨线程
  LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
  if (channel->index() < 0)      // 初始化时index=-1
  {
	  // index < 0说明是一个新的通道
    // a new one, add to pollfds_
    assert(channels_.find(channel->fd()) == channels_.end());   // 断言该fd没有被加入过，因为是新的
    
    /* 设置事件数组中对应的pollfd */
    struct pollfd pfd;
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    pollfds_.push_back(pfd);

    /* channel把fd在数组中的位置记录下来，map中记录下fd——channel键值对；建立双向连接 */
    int idx = static_cast<int>(pollfds_.size())-1;
    channel->set_index(idx);
    channels_[pfd.fd] = channel;
  }
  // update existing one
  else
  {
    
    assert(channels_.find(channel->fd()) != channels_.end());  // 断言可以在map中找到fd，因为是旧的
    assert(channels_[channel->fd()] == channel);    // 断言也可以通过map找到fd对应的channel，因为是旧的
    
    int idx = channel->index();    
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size())); // 断言channel对应的idx在给定范围内

    struct pollfd& pfd = pollfds_[idx];
    assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd()-1);  // 这里后者不好理解，这是暂不关注时的设置方式
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
	  // 将一个通道暂时更改为不关注事件，但不从Poller中移除该通道
    if (channel->isNoneEvent())
    {
	    // 暂时忽略该文件描述符的事件
      pfd.fd = -channel->fd()-1;	// 这样子设置是为了removeChannel优化
    }
  }
}

void PollPoller::removeChannel(Channel* channel)
{
  Poller::assertInLoopThread();
  LOG_TRACE << "fd = " << channel->fd();
  assert(channels_.find(channel->fd()) != channels_.end());
  assert(channels_[channel->fd()] == channel);
  assert(channel->isNoneEvent());
  int idx = channel->index();
  assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
  const struct pollfd& pfd = pollfds_[idx]; (void)pfd;
  assert(pfd.fd == -channel->fd()-1 && pfd.events == channel->events());
  size_t n = channels_.erase(channel->fd());
  assert(n == 1); (void)n;
  if (implicit_cast<size_t>(idx) == pollfds_.size()-1)
  {
    pollfds_.pop_back();
  }
  else
  {
	// 这里移除的算法复杂度是O(1)，将待删除元素与最后一个元素交换再pop_back
    int channelAtEnd = pollfds_.back().fd;
    iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
    if (channelAtEnd < 0)
    {
      channelAtEnd = -channelAtEnd-1;     // 得到原始的fd，从而找到对应的Channel来更新index
    }
    channels_[channelAtEnd]->set_index(idx);
    pollfds_.pop_back();
  }
}

