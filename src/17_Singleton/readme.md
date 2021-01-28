# 1.Singleton的核心--static
1.一个模版类T，我们不希望他被复制，被创建多次，所以我们需要把它放在Singleton类中，**把它作为一个static类型的成员，只被初始化一次；**
2.**Singleton作为一个包装类，构造一个或多个Singleton对象是无意义的，因为我们希望只能通过唯一的接口来获取唯一的T对象；所以我们把Singleton的构造函数，复制构造函数，赋值运算符等都设置为```private```；**
3.现在无法创建Singleton对象了，我们怎么去获取它的数据成员呢？**只需要定义一个static函数，用来返回T对象；因为是静态成员，所以可以直接通过类名，而不是通过对象来调用。**这样就无需创建一个Singleton对象，也能获得其静态成员了。
4.这种实现中，并没有限制直接创建多个没有被包装的T对象，因为它自身的构造函数是public的；所以只能用户有意识地，只使用被包装好的T对象。

# 2.pthread_once()
```cpp
int pthread_once(pthread_once_t *once_control, void (*init_routine) (void))；
```
使用互斥锁和条件变量保证指定的函数执行且仅执行一次，而once_control则表征是否执行过。如果once_control的初值不是PTHREAD_ONCE_INIT（Linux Threads定义为0），pthread_once()的行为就会不正常。在Linux Threads中，实际”一次性函数”的执行状态有三种：NEVER（0）、IN_PROGRESS（1）、DONE（2），如果 once初值设为1，则由于所有pthread_once()都必须等待其中一个激发”已执行一次”信号，因此所有pthread_once()都会陷入永久的等待中；如果设为2，则表示该函数已执行过一次，从而所有pthread_once()都会立即返回0。

# 3.::atexit()
```cpp
int atexit(void (* _Nonnull)(void)); // 其作用是注册某一个函数，当进程执行结束时，会自动调用注册的函数
```
我们在创建T时，通过此函数注册了destroy函数，在其中释放了动态内存```static T* value_```。
此操作不能放在Singleton的析构函数里，因为根本就没有创建任何Singleton对象，也不会执行析构函数，会造成内存泄漏。

# 4.通过char[-1]来提示error
```cpp
typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1]; 
```
很巧妙的写法