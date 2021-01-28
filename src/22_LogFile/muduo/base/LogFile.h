#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include <muduo/base/Mutex.h>
#include <muduo/base/Types.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{

class LogFile : boost::noncopyable
{
 public:
  LogFile(const string& basename,
          size_t rollSize,
          bool threadSafe = true,        // 默认是线程安全的，也就是加锁
          int flushInterval = 3);        // 刷新间隔
  ~LogFile();

  void append(const char* logline, int len);
  void flush();

 private:
  void append_unlocked(const char* logline, int len);

  static string getLogFileName(const string& basename, time_t* now);    // 产生新文件时调用，获取名字和时间
  void rollFile();      // 滚动日志，也就是产生新文件

  const string basename_;		// basename（不包含目录层级）
  const size_t rollSize_;		// rollSize_（满了之后就滚动）
  const int flushInterval_;		// 检查间隔

  int count_;           // 时间计数器

  boost::scoped_ptr<MutexLock> mutex_;   // 锁的智能指针
  time_t startOfPeriod_;	// 最新的文件是什么时候开始的（对齐每天的0点）
  time_t lastRoll_;			// 上次滚动是多久
  time_t lastFlush_;		// 上次清空缓存区是多久
  class File;         // 实际的日志文件类
  boost::scoped_ptr<File> file_;    // File智能指针成员

  const static int kCheckTimeRoll_ = 1024;      // count自增到1024就进行状态检查
  const static int kRollPerSeconds_ = 60*60*24;  
};

}
#endif  // MUDUO_BASE_LOGFILE_H
