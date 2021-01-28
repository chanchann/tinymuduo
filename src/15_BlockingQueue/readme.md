# 1.基于条件变量的生产者消费者模型 
没什么好说的，基本功要掌握扎实。各种细节，如控制锁的粒度，使用while判断条件而非if来避免假唤醒等等。 

# 2.一个使我眼前一亮的控制思想
消费者这边：设置一个bool变量，其为真时一直消费。
```cpp
void threadFunc()
  {
    printf("tid=%d, %s started\n",
           muduo::CurrentThread::tid(),
           muduo::CurrentThread::name());

    latch_.countDown();
    bool running = true;
    while (running)
    {
      std::string d(queue_.take());
      printf("tid=%d, get data = %s, size = %zd\n", muduo::CurrentThread::tid(), d.c_str(), queue_.size());
      running = (d != "stop");
    }

    printf("tid=%d, %s stopped\n",
           muduo::CurrentThread::tid(),
           muduo::CurrentThread::name());
  }
```

生产者这边：通过生产invalid数据来使bool变量为假，使得消费者跳出消费循环。   
```cpp
void joinAll()
  {
    for (size_t i = 0; i < threads_.size(); ++i)
    {
      queue_.put("stop");
    }

    for_each(threads_.begin(), threads_.end(), boost::bind(&muduo::Thread::join, _1));
  }
```