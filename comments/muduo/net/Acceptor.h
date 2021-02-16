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

//�μ����Գ���gaorongTest/Reactor_test07
//Accepter��ʵ����һ���򵥵�fd��channel��ֻ��������һЩ��ʼ���Ĺ���:bind,listen
//accepter��װ��fd��channel��������һЩ����
class Acceptor : boost::noncopyable
{
 public:
  typedef boost::function<void (int sockfd,
                                const InetAddress&)> NewConnectionCallback;

  Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  bool listenning() const { return listenning_; }
  void listen();

 private:
  void handleRead();

  EventLoop* loop_;			//channle�������¼�ѭ��
  Socket acceptSocket_;		//��Ӧ��socket
  Channel acceptChannel_;	//socket��channel
  NewConnectionCallback newConnectionCallback_;  //accrpt����õ��û��ص�����
  bool listenning_;			//�Ƿ��ڼ���״̬
  int idleFd_;				//�����ļ���������Ϊ�˷�ֹ�ļ������������
};

}
}

#endif  // MUDUO_NET_ACCEPTOR_H
