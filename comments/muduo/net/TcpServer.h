// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_TCPSERVER_H
#define MUDUO_NET_TCPSERVER_H

#include <muduo/base/Atomic.h>
#include <muduo/base/Types.h>
#include <muduo/net/TcpConnection.h>

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

namespace muduo
{
namespace net
{

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

///
/// TCP server, supports single-threaded and thread-pool models.
///
/// This is an interface class, so don't expose too much details.

//tcpServer����һ��accepter(����listenfd����channel_),
//����һ��EvenLoop�̳߳�(����TcpConnection�¼�)����һ�����Ӷ����map,
class TcpServer : boost::noncopyable
{

 //�μ����Գ���: gaorongTest/Reactor_test08  Reactor_test09
 
 public:
  typedef boost::function<void(EventLoop*)> ThreadInitCallback;
  enum Option
  {
    kNoReusePort,
    kReusePort,
  };

  //TcpServer(EventLoop* loop, const InetAddress& listenAddr);
  TcpServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg,
            Option option = kNoReusePort);
  ~TcpServer();  // force out-line dtor, for scoped_ptr members.

  const string& ipPort() const { return ipPort_; }
  const string& name() const { return name_; }
  EventLoop* getLoop() const { return loop_; }

  /// Set the number of threads for handling input.
  ///
  /// Always accepts new connection in loop's thread.
  /// Must be called before @c start
  /// @param numThreads
  /// - 0 means all I/O in loop's thread, no thread will created.
  ///   this is the default value.
  /// - 1 means all I/O in another thread.
  /// - N means a thread pool with N threads, new connections
  ///   are assigned on a round-robin basis.
  void setThreadNum(int numThreads);
  void setThreadInitCallback(const ThreadInitCallback& cb)
  { threadInitCallback_ = cb; }
  /// valid after calling start()
  boost::shared_ptr<EventLoopThreadPool> threadPool()
  { return threadPool_; }

  /// Starts the server if it's not listenning.
  ///
  /// It's harmless to call it multiple times.
  /// Thread safe.
  void start();   //����

  /// Set connection callback.
  /// Not thread safe.
  //�������ӵ��������ӹرյĻص�����
  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

  /// Set message callback.
  /// Not thread safe.
  //������Ϣ����ʱ�ص�����
  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_ = cb; }

 private:
  //���ӵ���ʱ����õ�һ������
  /// Not thread safe, but in loop
  void newConnection(int sockfd, const InetAddress& peerAddr);
  /// Thread safe.
  void removeConnection(const TcpConnectionPtr& conn);
  /// Not thread safe, but in loop
  void removeConnectionInLoop(const TcpConnectionPtr& conn);

  //���ӵ����ƺ������Ӷ����ָ����ɵ�map
  typedef std::map<string, TcpConnectionPtr> ConnectionMap;

  EventLoop* loop_;  // the acceptor loop , accept������evenLoop,��һ��������������EvenLoop
  const string ipPort_;   //��������ipport
  const string name_;     //������������
  boost::scoped_ptr<Acceptor> acceptor_; // avoid revealing Acceptor ������ָ�����ptr
  boost::shared_ptr<EventLoopThreadPool> threadPool_;  //EvenLoop�̳߳�
  ConnectionCallback connectionCallback_;  //���ӵ���ʱ�ص�����
  MessageCallback messageCallback_;			//��Ϣ����ʱ�ص�����
  WriteCompleteCallback writeCompleteCallback_;
  ThreadInitCallback threadInitCallback_;   //�̳߳س�ʼ���Ļص�����
  AtomicInt32 started_;   //�Ƿ��Ѿ�����
  // always in loop thread
  int nextConnId_;    //��һ������id
  ConnectionMap connections_;  //�����б�
};

}
}

#endif  // MUDUO_NET_TCPSERVER_H
