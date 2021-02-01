// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/Acceptor.h>

#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/stat.h>

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
  : loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie()), // 1.创建socket_fd
    acceptChannel_(loop, acceptSocket_.fd()),
    listenning_(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
  assert(idleFd_ >= 0);
  acceptSocket_.setReuseAddr(true);      // 设置端口复用
  acceptSocket_.bindAddress(listenAddr);      // 2.bind
  acceptChannel_.setReadCallback(
      boost::bind(&Acceptor::handleRead, this)); 
}

Acceptor::~Acceptor()
{
  acceptChannel_.disableAll();
  acceptChannel_.remove();
  ::close(idleFd_);
}

void Acceptor::listen()
{
  loop_->assertInLoopThread();
  listenning_ = true;
  acceptSocket_.listen();                // 3.listen
  acceptChannel_.enableReading();
}

//有新的客户端连接 -> socket_fd变为可读 -> poller返回该fd对应的socket_channel ->
// loop调用该channel的回调函数（本函数）-> 本函数中接受并建立连接，然后调用应用层的回调函数
void Acceptor::handleRead()   
{
  loop_->assertInLoopThread();
  InetAddress peerAddr(0);
  //FIXME loop until no more
  int connfd = acceptSocket_.accept(&peerAddr);
  if (connfd >= 0)
  {
    // string hostport = peerAddr.toIpPort();
    // LOG_TRACE << "Accepts of " << hostport;
    if (newConnectionCallback_)
    {
      newConnectionCallback_(connfd, peerAddr);
    }
    else
    {
      sockets::close(connfd);
    }
  }
  else
  {
    // Read the section named "The special problem of
    // accept()ing when you can't" in libev's doc.
    // By Marc Lehmann, author of livev.
    if (errno == EMFILE)   // 说明连接到达上限。如果不处理，那么水平模式下会一直触发可读事件
    {
      ::close(idleFd_);    // 关闭备用的fd
      idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);  // 接受新的连接
      ::close(idleFd_);  // 关闭该连接（相当于读走了数据）
      idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);  // 重新打开备用fd
    }
  }
}

