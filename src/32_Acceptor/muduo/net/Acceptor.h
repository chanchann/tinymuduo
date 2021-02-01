// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <muduo/net/Channel.h>
#include <muduo/net/Socket.h>

namespace muduo
{
namespace net
{

class EventLoop;
class InetAddress;

///
/// Acceptor of incoming TCP connections.
///
class Acceptor : boost::noncopyable
{
 public:
  typedef boost::function<void (int sockfd,
                                const InetAddress&)> NewConnectionCallback;   // 应用层回调函数

  Acceptor(EventLoop* loop, const InetAddress& listenAddr);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  bool listenning() const { return listenning_; }
  void listen();

 private:
  void handleRead();

  EventLoop* loop_;
  Socket acceptSocket_;          //  listen_fd所在socket
  Channel acceptChannel_;       // listen_fd所在channel
  NewConnectionCallback newConnectionCallback_;  // 基于对象，包含一个可设置的回调函数
  bool listenning_;
  int idleFd_;   // 为解决水平触发模式下，连接过多导致的的持续可读
};

}
}

#endif  // MUDUO_NET_ACCEPTOR_H
