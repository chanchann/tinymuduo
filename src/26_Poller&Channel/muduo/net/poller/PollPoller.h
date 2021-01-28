// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_POLLER_POLLPOLLER_H
#define MUDUO_NET_POLLER_POLLPOLLER_H

#include <muduo/net/Poller.h>

#include <map>
#include <vector>

/*
struct pollfd {
	int     fd;         //文件描述符，但不负责关闭该文件描述符
	short   events;     // 关注的事件	
	short   revents;    // poll返回的事件
};
*/
struct pollfd;


namespace muduo
{
namespace net
{

///
/// IO Multiplexing with poll(2).
///
class PollPoller : public Poller
{
 public:

  PollPoller(EventLoop* loop);
  virtual ~PollPoller();

  virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels);
  virtual void updateChannel(Channel* channel);
  virtual void removeChannel(Channel* channel);

 private:
  void fillActiveChannels(int numEvents,
                          ChannelList* activeChannels) const;   // 将有对应事件发生的pollfd的channel集中放到activeChannels

  typedef std::vector<struct pollfd> PollFdList;
  typedef std::map<int, Channel*> ChannelMap;	// key是文件描述符fd，value是Channel*。已知fd，找到对应的channel
  PollFdList pollfds_;
  ChannelMap channels_;
};

}
}
#endif  // MUDUO_NET_POLLER_POLLPOLLER_H
