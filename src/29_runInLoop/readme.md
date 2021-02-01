# 1.eventfd
其实就是将一个```fd```作为唤醒的工具。
* 当IO线程自己需要IO线程做一些任务时，它将按时自己执行；
* 当其他线程自己需要IO线程做一些任务时，就将任务添加到队列中，向```eventfd```中写入数据，唤醒IO线程，使其去执行添加的任务。

# 2.```queueInLoop(const Functor& cb)```唤醒的时机
有以下三种情况会调用该函数添加任务：
* 1. 其他线程调用```runInLoop()```。此时需要唤醒IO线程，避免其阻塞在```epoll_wait()```上。
* 2. IO线程在处理任务时，其中要求去调用```queueInLoop(const Functor& cb)```添加任务。此时也需要唤醒IO线程，否则新任务在下次唤醒之前没机会执行。
* 3. 普通的```Channel```中，调用```queueInLoop(const Functor& cb)```添加任务。此时不需要唤醒，因为处理完普通的```Channel```后才会去处理添加的任务，所以之前添加的任务一定能执行。

# 3. ```doPendingFunctors()```中的细节
```cpp
{
  MutexLockGuard lock(mutex_);
  functors.swap(pendingFunctors_);
}
```
采用交换而不是直接在临界区处理任务：
* 1.由于交换之后就可以解锁，所以不会阻塞其他线程调用```queueInLoop(const Functor& cb)```添加任务。
* 2.由于```Functors```中也可能调用```queueInLoop(const Functor& cb)```添加任务，所以交换后及时解锁，再去处理任务，可以避免死锁。

这里没有一直处理任直到```pendingFunctors_```为空，因为要避免其在处理任务时进入死循环，反而没时间去处理IO事件了。