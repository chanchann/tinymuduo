#include <muduo/base/LogFile.h>
#include <muduo/base/Logging.h> // strerror_tl
#include <muduo/base/ProcessInfo.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace muduo;

// not thread safe
class LogFile::File : boost::noncopyable       // 这个是嵌套类，线程安全由外层实现
{
 public:
  explicit File(const string& filename)
    : fp_(::fopen(filename.data(), "ae")),    
      writtenBytes_(0)
  {
    assert(fp_);
    ::setbuffer(fp_, buffer_, sizeof buffer_); // 在打开文件流后，可用来设置文件流的缓冲区。buf指向自定的缓冲区起始地址。
    // posix_fadvise POSIX_FADV_DONTNEED ?
  }

  ~File()
  {
    ::fclose(fp_);
  }

  void append(const char* logline, const size_t len)   // 和网络编程中非阻塞模式write()很像————没有写完就要一直写
  {
    size_t n = write(logline, len);
    size_t remain = len - n;     // 还剩多少没写
	  // remain>0
    while (remain > 0)     // 没写完一直写
    {
      size_t x = write(logline + n, remain);
      if (x == 0)         // 说明文件已经关闭
      {
        int err = ferror(fp_);
        if (err)
        {
          fprintf(stderr, "LogFile::File::append() failed %s\n", strerror_tl(err));
        }
        break;
      }
      n += x;
      remain = len - n; // remain -= x
    }

    writtenBytes_ += len;     // 保存已经写入的字节数
  }

  void flush()
  {
    ::fflush(fp_);
  }

  size_t writtenBytes() const { return writtenBytes_; }

 private:

  size_t write(const char* logline, size_t len)
  {
#undef fwrite_unlocked
    // MacOS不提供不加锁版本的fwrite()，这里会造成一点性能损失
    // return ::fwrite_unlocked(logline, 1, len, fp_);         这里可以用不加锁版本，是因为外部调用时会加锁，保证安全
    return ::fwrite(logline, 1, len, fp_);
  }

  FILE* fp_;
  char buffer_[64*1024];     
  size_t writtenBytes_;      // 已经写入缓冲区的字节数
};

LogFile::LogFile(const string& basename,
                 size_t rollSize,
                 bool threadSafe,
                 int flushInterval)
  : basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    count_(0),
    mutex_(threadSafe ? new MutexLock : NULL),   // 依据threadSafe确定要不要提供锁
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0)
{
  assert(basename.find('/') == string::npos);  // 判断是否取basename成功
  rollFile();    // 滚动生成新日志文件
}

LogFile::~LogFile()
{
}

void LogFile::append(const char* logline, int len)
{
  if (mutex_)   
  {
    MutexLockGuard lock(*mutex_);  // 先加锁再写
    append_unlocked(logline, len);
  }
  else
  {
    append_unlocked(logline, len);  // 直接写
  }
}

void LogFile::flush()
{
  if (mutex_)
  {
    MutexLockGuard lock(*mutex_);    // 先加锁再刷新
    file_->flush();
  }
  else
  {
    file_->flush();     // 直接刷新
  }
}

void LogFile::append_unlocked(const char* logline, int len)
{
  file_->append(logline, len);

  /* 写了之后要检查，是否需要产生新日志文件 */
  if (file_->writtenBytes() > rollSize_)   // 如果写的数据到达阈值，产生新文件
  {
    rollFile();   // 新文件中writtenBytes重置为0
  }
  else
  {
    if (count_ > kCheckTimeRoll_)     // count自增到1024就进行状态检查 
    {
      count_ = 0;
      /* 对齐thisPeriod到当天0点，如果过了一天，不论是否写满，直接滚动 */
      time_t now = ::time(NULL);
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;   // 这里由于都是long类型，进行的是对齐操作
      if (thisPeriod_ != startOfPeriod_)
      {
        rollFile();
      }
      /* 刷新间隔（默认3s）到了，就进行刷新 */
      else if (now - lastFlush_ > flushInterval_)
      {
        lastFlush_ = now;
        file_->flush();
      }
    }
    else
    {
      ++count_;    
    }
  }
}

void LogFile::rollFile()
{
  time_t now = 0;
  string filename = getLogFileName(basename_, &now);
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;   // 对齐到当天0点

  if (now > lastRoll_)   // 防止多线程时，多次生成新文件
  { 
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = start;
    file_.reset(new File(filename));    // reset()实际上是swap操作，自己的指针和临时对象指针的交换，原指针就在临时对象析构时被销毁了
  }
}

string LogFile::getLogFileName(const string& basename, time_t* now)    // 获取名字，保存格式化时间等数据
{
  string filename;
  filename.reserve(basename.size() + 64);
  filename = basename;

  char timebuf[32];
  char pidbuf[32];
  struct tm tm;
  *now = time(NULL);
  gmtime_r(now, &tm); // FIXME: localtime_r ?
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
  filename += timebuf;
  filename += ProcessInfo::hostname();
  snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
  filename += pidbuf;
  filename += ".log";

  return filename;
}

