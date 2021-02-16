// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include <muduo/base/StringPiece.h>
#include <muduo/base/Types.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>

#include <boost/any.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace muduo
{
namespace net
{

class Channel;
class EventLoop;
class Socket;

///
/// TCP connection, for both client and server usage.
///
/// This is an interface class, so don't expose too much details.

//TcpConnection����һ������
//����������ӵ����������������
class TcpConnection : boost::noncopyable,
                      public boost::enable_shared_from_this<TcpConnection>
{

//����̳���enable_shared_from_this��Ϊ��ʹ��shared_from_this����
//shared_from_this�������Ա�֤���õĹ��������ü�����һ���������´���һ������
//�μ�����gaorongTest/Esft.cpp


//TcpConnection�����������ͷŹ���:
//��һ��tcp���ӹرյ�ʱ�����ȴ���Poller�Ŀɶ��¼�������Channel::handledEvent������
//Ȼ�����TcpConnection::handledread����handledRead����read���ַ����ֽ�Ϊ0
//��������TcpCOnnecton::handledClose��������������tcpServerע��Ļص�����removeConnection
//TcpServer::removeConnection�Ὣ���TcpConnection�����������б���ɾ��
//���Ǵ�ʱ����ֱ��delete TcpConnection���������ΪTcpConnnection������
//��channel���ں�������ջ�е���handledEvent����ʱ���ֱ��delete TcpConnection
//�ὫchannelҲdelete������������core dump����������Ҫ��channel::handeledEvent
//ִ�����֮����delete TcpConnection��������õ�shared_ptr��



 public:
  /// Constructs a TcpConnection with a connected sockfd
  ///
  /// User should not create this object.
  TcpConnection(EventLoop* loop,
                const string& name,
                int sockfd,
                const InetAddress& localAddr,
                const InetAddress& peerAddr);
  ~TcpConnection();

  EventLoop* getLoop() const { return loop_; }
  const string& name() const { return name_; }
  const InetAddress& localAddress() const { return localAddr_; }
  const InetAddress& peerAddress() const { return peerAddr_; }
  bool connected() const { return state_ == kConnected; }
  bool disconnected() const { return state_ == kDisconnected; }
  // return true if success.
  bool getTcpInfo(struct tcp_info*) const;
  string getTcpInfoString() const;

  // void send(string&& message); // C++11
  void send(const void* message, int len);
  void send(const StringPiece& message);
  // void send(Buffer&& message); // C++11
  void send(Buffer* message);  // this one will swap data
  void shutdown(); // NOT thread safe, no simultaneous calling
  // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no simultaneous calling

  void forceClose();
  void forceCloseWithDelay(double seconds);
  void setTcpNoDelay(bool on);
  // reading or not
  void startRead();
  void stopRead();
  bool isReading() const { return reading_; }; // NOT thread safe, may race with start/stopReadInLoop

  void setContext(const boost::any& context)
  { context_ = context; }

  const boost::any& getContext() const
  { return context_; }

  boost::any* getMutableContext()
  { return &context_; }

  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_ = cb; }

  void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
  { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

  /// Advanced interface
  Buffer* inputBuffer()
  { return &inputBuffer_; }

  Buffer* outputBuffer()
  { return &outputBuffer_; }

  
  /// Internal use only.  ֻ���ڲ�ʹ�ã���TcpServerע�ᣬ�����û�
  void setCloseCallback(const CloseCallback& cb)
  { closeCallback_ = cb; }

  // called when TcpServer accepts a new connection
  void connectEstablished();   // should be called only once
  // called when TcpServer has removed me from its map
  void connectDestroyed();  // should be called only once

 private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();
  // void sendInLoop(string&& message);
  void sendInLoop(const StringPiece& message);
  void sendInLoop(const void* message, size_t len);
  void shutdownInLoop();
  // void shutdownAndForceCloseInLoop(double seconds);
  void forceCloseInLoop();
  void setState(StateE s) { state_ = s; }
  const char* stateToString() const;
  void startReadInLoop();
  void stopReadInLoop();

  EventLoop* loop_;
  const string name_;  //��������
  //����״̬��ö������
  StateE state_;  // FIXME: use atomic variable  
  bool reading_;
  // we don't expose those classes to client.
  boost::scoped_ptr<Socket> socket_;
  boost::scoped_ptr<Channel> channel_;
  const InetAddress localAddr_;   //���ص�ַ
  const InetAddress peerAddr_;	  //�Եȵ�ַ
  
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;     // ���ݷ�����ϻص������������е��û����ݶ��ѿ������ں˻�����ʱ�ص��ú���
													// outputBuffer_�����Ҳ��ص��ú������������Ϊ��ˮλ��ص�����
  HighWaterMarkCallback highWaterMarkCallback_;    // ��ˮλ��ص�����,Ҳ����outputBuffer�ŵ�һ���̶���
  CloseCallback closeCallback_;

  size_t highWaterMark_;	//��ˮλ��
  Buffer inputBuffer_;		//���ջ�����
  Buffer outputBuffer_; // FIXME: use list<Buffer> as output buffer.

  //boost::any��һ�ֿɱ����͵�ָ�룬��void*���Ͱ�ȫ����֧���������͵����Ͱ�ȫ�洢�Լ���ȫ����
  //�����ڱ�׼�������д�Ų�ͬ���͵ķ���������vector<boost::any>
  boost::any context_;	//��һ��δ֪���͵������Ķ���
  // FIXME: creationTime_, lastReceiveTime_
  //        bytesReceived_, bytesSent_
};

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

}
}

#endif  // MUDUO_NET_TCPCONNECTION_H
