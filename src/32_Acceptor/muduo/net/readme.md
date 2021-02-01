# 1.```loop->runInLoop(const Functor& cb)```
调用此函数时，```loop```会调用```isInLoopThread()```，来检查```EventLoop```中储存的```tid```和```CurrentThread::tid()```获取到的```tid```是否相等。

由于```loop```指针是在主线程中创建的，虽然其指向IO线程中的```EventLoop```对象，但其调用```runInLoop()```是在主线程内，所以其调用```CurrentThread::tid()```获取到的是主线程的```tid```，一定和新建的IO线程的```tid```不相同，所以会将任务加入到队列，使用异步调用方式交给IO线程完成。


# 2. ```EventLoop* loop_```的传递
```EventLoopThread```中有一个指针```EventLoop* loop_```，在创建新线程后，新的线程会创建一个```EventLoop```对象，将其赋给```loop_```指针———之所以能传递，是因为创建新线程时指定的回调函数，绑定了```this```指针，也就是将指向```EventLoopThread```的指针传递了进去。这样才能完成赋值；而```EventLoopThread```将该指针```loop_```在```startLoop()```中返回，便于在主线程中对IO线程进行异步调用。