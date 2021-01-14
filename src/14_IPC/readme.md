# 1. RAII
RAII的做法是使用一个对象，在其构造时获取资源，在对象生命期控制对资源的访问使之始终保持有效，最后在对象析构的时候释放资源。
**不止用来管理内存泄漏，还可以用来防止端口泄漏，防止死锁。实际上，任何需要一头一尾操作的对象都可以这样封装（open/close等）**

# 2.MutexLockGuard与MutexLock
MutexLockGuard中的成员变量是MutexLock&类型，只是通过该引用使用了MutexLock的lock()与unlock()方法，和MutexLock并没有局部和整体的关系，也并不负责MutexLock的生命周期；所以二者只是关联关系。

另外，作者给出这样的提示：
```cpp
// Prevent misuse like:
// MutexLockGuard(mutex_);
// A tempory object doesn't hold the lock for long!
#define MutexLockGuard(x) error "Missing guard object name"

#endif  // MUDUO_BASE_MUTEX_H
```
上述的做法，产生了一个MutexLockGuard的匿名临时量，一个临时量并不能长时间拥有一把锁，生成之后即被销毁，是无意义的；所以将上述用法宏定义为error

# 3.boost::bind()
## 可以把成员函数作为普通函数对象
在构造```Thread```类时，它接收```ThreadFunc```类型，即```void()```类型的函数来初始化.
```cpp
typedef boost::function<void()> ThreadFunc; 
explicit Thread(const ThreadFunc&, const string& name = string());
```
而```Test::threadFunc```是```void(*Test)```类型的函数，多了一个this指针，需要用```boost::bind()```转换接口
```cpp
auto foo = boost::bind(&Test::threadFunc, this); // this被绑定了，foo()的函数类型就是void()了

boost::bind(&Thread::start, _1)(Thread* t);   // 通过占位符，在调用时确定参数的值和位置；下面三者等价
Thread::start(t); 
t->start();
```

# 4.经典写法：for_each() + boost::bind()
```cpp
for_each(iterator begin, iterator end, func)；// func为可调用对象；上述函数等价于：
for(iter it = beg;it != end;++it)
{
    func(it);
}
```
### 两者结合，调用容器中类类型成员变量的成员函数
```cpp
for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::start, _1));   // _1代表传入的指针的位置
for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::join, _1));
```

# 5.利用封装之后的条件变量
1.子线程等待主线程的命令
2.主线程等待子线程准备好