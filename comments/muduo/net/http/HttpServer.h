// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_HTTP_HTTPSERVER_H
#define MUDUO_NET_HTTP_HTTPSERVER_H

#include <muduo/net/TcpServer.h>
#include <boost/noncopyable.hpp>

namespace muduo
{
namespace net
{

class HttpRequest;
class HttpResponse;

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.

//http�������ࡣ������һ��TcpServer����tcpServer֮�Ϸ�װ���Լ����߼�
//������tcpServerע��onConnection,��onMessage��������tcp����ʱ��ص��ú���
//����ص��û�ע���httpCallback_�������ú�������HttpRequest,ָ�봫��һ��HttpResponse
//�����ͨ��HttpServer���ظ��û�
class HttpServer : boost::noncopyable
{
 public:
  typedef boost::function<void (const HttpRequest&,
                                HttpResponse*)> HttpCallback;

  HttpServer(EventLoop* loop,
             const InetAddress& listenAddr,
             const string& name,
             TcpServer::Option option = TcpServer::kNoReusePort);

  ~HttpServer();  // force out-line dtor, for scoped_ptr members.

  EventLoop* getLoop() const { return server_.getLoop(); }

  /// Not thread safe, callback be registered before calling start().
  void setHttpCallback(const HttpCallback& cb)
  {
    httpCallback_ = cb;
  }

  void setThreadNum(int numThreads)
  {
    server_.setThreadNum(numThreads);
  }

  void start();

 private:
  void onConnection(const TcpConnectionPtr& conn);
  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp receiveTime);
  void onRequest(const TcpConnectionPtr&, const HttpRequest&);

  TcpServer server_;
  HttpCallback httpCallback_;  // �ڴ���http���󣨼�����onRequest���Ĺ����лص��˺�������������о���Ĵ���
};

}
}

#endif  // MUDUO_NET_HTTP_HTTPSERVER_H