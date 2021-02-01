# 1.TcpServer的构成
主要由```Acceptor```和一个```TcpConnection```列表构成。

# 2.建立连接处理顺序
#### 1.```loop()```循环中由```poller```返回活跃的监听通道```Channel```
#### 2.调用```Channel```的成员函数```handleEvent()```，其中调用```readCallback_()```
#### 3.对于```Acceptor```而言，其中调用注册的回调函数````Acceptor::handleRead()```。内部先是调用```accept()```建立连接，返回新的```conn_fd```，然后调用回调函数```newConnectionCallback_()(一般应用层不直接使用该函数)```
#### 4.```TcpServer```将```Acceptor```的```newConnectionCallback_()```，设置为自己的```TcpServer::newConnection()```。其中创建一个新的```TcpConnection```，根据```conn_fd```设置```conn```的名字，将```conn```加入```TcpConnection```列表，并且设置其不同情况下的回调函数。然后调用```conn->connectEstablished()```。
#### 5.```conn->connectEstablished()```中，```conn_fd```所在的```Channel```会将自己放入监听队列。然后调用```TcpConnection::connectionCallback_()```，代表整个连接建立完成。

# 3.断开连接处理顺序
由于断开连接也是触发可写事件，所以前面的流程和建立连接时类似：
#### 1.```loop()```循环中由```poller```返回活跃的监听通道```Channel```
#### 2.调用```Channel```的成员函数```handleEvent()```，其中调用```readCallback_()```
#### 3.对于```TcpConnection```而言，是调用注册的回调函数```TcpConnection::handleRead()```。在其中，由于对端关闭，所以read函数会返回0，因此，会调用```TcpConnection::handleClose()```。
#### 4.其中，会设置其状态为```kDisconnected```，并且调用其```TcpConnection::closeCallback_```;```TcpServer```在创建```TcpConnection```时，将其设置为自己的```TcpServer::removeConnection()```。
#### 5.其中，会将该连接从```TcpConnection```列表中移除（**此时该对象并未销毁**）；然后调用```loop_->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn))```，将真正销毁该对象的任务，添加到IO线程的任务队列中
#### 6.```loop()```循环中，会移除其```Channel```，回调应用层的函数，在函数结束后，计数为0，该```TcpConnection```对象真正被销毁

**为什么第5步中不直接delete掉TcpConnection对象呢？因为此时还在```Channel::handleEvent()```中，如果delete掉连接，那么```Channel```自身也被销毁了，会导致core down错误。所以，要在```Channel::handleEvent()```执行完之后，由```loop()```循环来执行销毁操作。**

# 4.TcpConnectionPtr引用计数的变化过程
这里不一一解释，重点在代码对应行号处阐明。
```cpp
TcpServer::newConnection [TestServer] - new connection [TestServer:0.0.0.0:8888#1] from 127.0.0.1:41360 - TcpServer.cc:86
TcpConnection TcpConnection::ctor[TestServer:0.0.0.0:8888#1] at 0xCE8630 fd=8 - TcpConnection.cc:62
newConnection [1] usecount=1 - TcpServer.cc:104
newConnection [2] usecount=2 - TcpServer.cc:106
connectEstablished [3] usecount=3 - TcpConnection.cc:78
updateChannel fd = 8 events = 3 - EPollPoller.cc:104
onConnection(): new connection [TestServer:0.0.0.0:8888#1] from 127.0.0.1:41360
connectEstablished [4] usecount=3 - TcpConnection.cc:83
newConnection [5] usecount=2 - TcpServer.cc:116
poll 1 events happended - EPollPoller.cc:65
printActiveChannels {8: IN }  - EventLoop.cc:257
handleEvent [6] usecount=2 - Channel.cc:67
handleClose fd = 8 state = 2 - TcpConnection.cc:145
updateChannel fd = 8 events = 0 - EPollPoller.cc:104
onConnection(): connection [TestServer:0.0.0.0:8888#1] is down
handleClose [7] usecount=3 - TcpConnection.cc:153
TcpServer::removeConnectionInLoop [TestServer] - connection TestServer:0.0.0.0:8888#1 - TcpServer.cc:123
removeConnection [8] usecount=3 - TcpServer.cc:127
removeConnection [9] usecount=2 - TcpServer.cc:129
removeConnection [10] usecount=3 - TcpServer.cc:136
handleClose [11] usecount=3 - TcpConnection.cc:156
handleEvent [12] usecount=2 - Channel.cc:69
removeChannel fd = 8 - EPollPoller.cc:147
~TcpConnection TcpConnection::dtor[TestServer:0.0.0.0:8888#1] at 0xCE8630 fd=8 - TcpConnection.cc:69
```
