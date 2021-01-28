# 1.把握日志生成到输出的整体流程
主要思想是：
1.通过宏定义，生成与LogLevel对应的Logger匿名对象，其中封装有Impl对象；
2.Impl对象中封装有LogStream对象，其中有一个Buffer。Impl在初始化时，就会完成信息到Buffer的输出————时间，tid，LogLevel，如果有errno生成也会记录。
3.我们无法对匿名临时对象进行操作，所以最后的输出工作在Logger的析构函数中进行。它会完成输出到指定位置、刷新的工作。

总体而言，```LogStream```负责保存和管理生成的数据，```Impl```负责一些数据的格式化，```Logger```负责最外层的一些接口和等级指定等。

# 2.Logger的具体流程
```cpp
User -> Logger -> Impl -> LogStream -> operator<<FixedBuffer -> g_output -> g_flush -> User
```
以下面的宏为例：
```cpp
#define LOG_INFO if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
  muduo::Logger(__FILE__, __LINE__).stream()

LOG_INFO<<"info";

Logger::~Logger()            // 从buf到真正的输出位置，在析构函数中完成
{
  impl_.finish();
  const LogStream::Buffer& buf(stream().buffer());     
  g_output(buf.data(), buf.length());
  if (impl_.level_ == FATAL)       // 如果是fatal error，终止程序
  {
    g_flush();
    abort();
  }
}
```
上述语句：
1.通过所在文件名，调用时所在行号，创建了一个匿名对象；
2.通过```stream()```获取到```Impl```中封装的```LogStream```;
3.通过```LOG_INFO<<"info";```，调用```LogStream```中重载的对应的运算符，完成对Buffer的输出；
4.```LOG_INFO<<"info";```执行过后，会调用```Logger```的析构函数，在其中调用```g_output();```和```g_flush()```，完成最终的输出任务。

# 3.FixBuffer的实现
**很经典的实现办法，建议熟悉一下**
```cpp
template<int SIZE>
class FixedBuffer : boost::noncopyable
{
 public:
  FixedBuffer()
    : cur_(data_){}

  ~FixedBuffer(){}

  void append(const char* /*restrict*/ buf, size_t len)
  {
    // FIXME: append partially    暂未实现部分传入
    if (implicit_cast<size_t>(avail()) > len)
    {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }
 
  const char* data() const { return data_; }            // 获取data
  int length() const { return static_cast<int>(cur_ - data_); }        // 获取长度

  // write to data_ directly
  char*     current() { return cur_; }
  int avail() const { return static_cast<int>(end() - cur_); }     // 目前剩余的空间
  void add(size_t len) { cur_ += len; }            // 移动cur指针的位置

  void reset() { cur_ = data_; }     
  void bzero() { ::bzero(data_, sizeof data_); }       // 清空data

  // for used by GDB
  const char* debugString();         // 作为字符串输出
  void setCookie(void (*cookie)()) { cookie_ = cookie; }
  // for used by unit test
  string asString() const { return string(data_, length()); }        // 作为string输出

 private:
  const char* end() const { return data_ + sizeof data_; }          // data尾后指针
  char data_[SIZE];    // 字符数组
  char* cur_;     // 当前位置
};
```

# 4.StringPiece类的实现
**几个要点**
1.通过简单的指针操作，可以接收"const char*" or a "string"作为传入参数，同时避免内存的拷贝；
2.使用宏定义来实现比较运算符（类似于测试框架的做法），很有意思，效率也很高：
```cpp
#define STRINGPIECE_BINARY_PREDICATE(cmp,auxcmp)                             \
  bool operator cmp (const StringPiece& x) const {                           \
    int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_); \
    return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_)));          \
  }
  STRINGPIECE_BINARY_PREDICATE(<,  <);
  STRINGPIECE_BINARY_PREDICATE(<=, <);
  STRINGPIECE_BINARY_PREDICATE(>=, >);
  STRINGPIECE_BINARY_PREDICATE(>,  >);
#undef STRINGPIECE_BINARY_PREDICATE
```