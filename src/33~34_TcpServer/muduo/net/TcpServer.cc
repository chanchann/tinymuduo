// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/TcpServer.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Acceptor.h>
#include <muduo/net/EventLoop.h>
//#include <muduo/net/EventLoopThreadPool.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <stdio.h>  // snprintf

using namespace muduo;
using namespace muduo::net;

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const string& nameArg)
  : loop_(CHECK_NOTNULL(loop)),
    hostport_(listenAddr.toIpPort()),
    name_(nameArg),
    acceptor_(new Acceptor(loop, listenAddr)),
    /*threadPool_(new EventLoopThreadPool(loop)),
    connectionCallback_(defaultConnectionCallback),
    messageCallback_(defaultMessageCallback),*/
    started_(false),
    nextConnId_(1)
{
  // Acceptor::handleRead函数中会回调用TcpServer::newConnection
  // _1对应的是socket文件描述符，_2对应的是对等方的地址(InetAddress)。占位符
  acceptor_->setNewConnectionCallback(
      boost::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
  loop_->assertInLoopThread();
  LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

  for (ConnectionMap::iterator it(connections_.begin());
      it != connections_.end(); ++it)
  {
    TcpConnectionPtr conn = it->second;
    it->second.reset();		// 释放当前所控制的对象，引用计数减一
    conn->getLoop()->runInLoop(
      boost::bind(&TcpConnection::connectDestroyed, conn));
    conn.reset();			// 释放当前所控制的对象，引用计数减一
  }
}

// 该函数多次调用是无害的（会先判断started_）
// 该函数可以跨线程调用
void TcpServer::start()
{
  if (!started_)
  {
    started_ = true;
  }

  if (!acceptor_->listenning())
  {
	// get_pointer返回原生指针。这里能保证Acceptor::listen()中不会造成原生指针的销毁。
    loop_->runInLoop(
        boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
  }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
  loop_->assertInLoopThread();

  /* 设置连接名 */
  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", hostport_.c_str(), nextConnId_);
  ++nextConnId_;
  string connName = name_ + buf;

  LOG_INFO << "TcpServer::newConnection [" << name_
           << "] - new connection [" << connName
           << "] from " << peerAddr.toIpPort();
  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  /* 创建新的shared_ptr，最好使用make_shared()
  TcpConnectionPtr conn = make_shared(new TcpConnection(loop_,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));
  */                                      
  TcpConnectionPtr conn(new TcpConnection(loop_,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));

  LOG_TRACE << "[1] usecount=" << conn.use_count(); // 刚刚创建，计数为1
  connections_[connName] = conn;  // 加入map
  LOG_TRACE << "[2] usecount=" << conn.use_count(); // 加入map时调用拷贝构造，计数++为2

  /* 设置几个回调函数 */
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);

  conn->setCloseCallback(
      boost::bind(&TcpServer::removeConnection, this, _1));

  conn->connectEstablished();
  LOG_TRACE << "[5] usecount=" << conn.use_count(); // 上一函数结束，计数为2
  // 此函数结束后，局部对象conn被销毁；计数--为1
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
           << "] - connection " << conn->name();


  LOG_TRACE << "[8] usecount=" << conn.use_count(); //此时依然为3
  size_t n = connections_.erase(conn->name());
  LOG_TRACE << "[9] usecount=" << conn.use_count();  // erase后，map中对象被销毁；计数--为2

  (void)n;
  assert(n == 1);
  
  loop_->queueInLoop(
      boost::bind(&TcpConnection::connectDestroyed, conn));

  // 值传递到TcpConnection::connectDestroyed()中，计数++为3    
  LOG_TRACE << "[10] usecount=" << conn.use_count();  
}
