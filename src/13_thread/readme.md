# 1. __thread标识符
GCC内置的线程局部储存设施：该标识符将全局变量从线程间共享，缩小范围到线程内全局可见（每个线程有自己单独的一份）
**适用范围**：只能修饰POD(plain old data)类型，即与C兼容的原始数据类型（带有用户定义的构造函数或者虚函数的类则不是）
```cpp
__thread string t_obj1 = string("cpp")  //错误：不能调用对象的构造函数
__thread string* t_obj2 = new string  //错误：初始化只能是编译期常量
__thread string* t_obj3 = NULL  //正确
```

# 2. pthread_atfork()
```cpp
// int pthread_atfork(void (* prepare)(void), void (* parent)(void), void (* child)(void));
// 内部创建子进程前，父进程会调用prepare；创建成功后，父进程调用parent，子进程调用child （注意：这里是进程）
pthread_atfork(NULL, NULL, &afterFork);   // fork成功后，子进程调用afterfork()
```
这样多进程多线程混合使用不好，可能会造成死锁，场景如下：
1.父进程pthread_create()了一个新的线程，并在其中对mutex上锁，使用资源；
2.父进程在此期间fork了一个子进程，它复制了父进程的内存空间（mutex是lock状态）；但是只会复制当前线程的执行状态（并没有另一个子线程来解锁）。
3.此时在子进程中，只要它调用mutex_lock()，就会等待一把已上锁且不会解锁的锁，也就是死锁状态。
除非使用```pthread_atfork()```在fork前先解锁，再复制内存。

