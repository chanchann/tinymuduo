# tinymuduo

https://blog.csdn.net/voidccc/article/details/8719752

https://www.cnblogs.com/secondtonone1/p/7076769.html

https://blog.csdn.net/NK_test/article/details/51052539

https://www.youtube.com/watch?v=b7n3ZELWGlM&list=PLwIrqQCQ5pQl6qfekzrQ2ZxXAlIGbLnRe

网络编程实践 陈硕

https://www.bilibili.com/video/BV1ez4y197jN?from=search&seid=8550822479399282544

## muduo库epoll为什么没用edge-trigger？

https://www.zhihu.com/question/275757062

https://github.com/chenshuo/muduo/issues/234

## 注意

```c 
typedef union epoll_data
{
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;
```

这里是union,不能又设置ptr,又设置fd

## Eventloop.h/cc 中wakeup()

https://cloud.tencent.com/developer/article/1008515

http://blog.xyecho.com/muduo-8-muduo-eventloop-threads/

## 匿名namespace 

https://blog.csdn.net/Solstice/article/details/6186978

比如 boost 里就常常用 boost::detail 来放那些“不应该暴露给客户，但又不得不放到头文件里”的函数或 class。

## wakeupchannle 

https://zhuanlan.zhihu.com/p/34719855