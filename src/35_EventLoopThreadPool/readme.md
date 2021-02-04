# 1.多IO线程TcpServer的构成
主要由一个```base loop```来管理```Acceptor```和IO线程池；线程池中其他线程来处理客户端的请求。

# 2.EventLoopThreadPool的实现
首先我们在```TcpServer```的线程中创建一个```base loop```。
之前我们实现了```EventLoopThread```类，它可以创建一个新线程，并且在该新线程中创建一个```EventLoop```循环。
当我们在创建```TcpServer```时指定了```numThreads```，我们就循环创建```EventLoopThread```类这么多次，生成对应个```EventLoop```循环。
当有新连接到来，```TcpServer::newConnection()```不在是在当前线程中添加```channel```，而是通过```base loop```用轮叫的方式，获取一个```EventLoop```，并添加到该```EventLoop```中。之后该连接的事件，就由该线程来处理了，而不是让```base loop```来处理。

要注意，所有不能跨线程调用的函数，都需要加入到所属线程的任务队列中执行。

```cpp
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
```
