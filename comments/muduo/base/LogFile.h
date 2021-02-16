#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include <muduo/base/Mutex.h>
#include <muduo/base/Types.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{

namespace FileUtil
{
class AppendFile;
}

//��һ���̰߳�ȫ����־��
class LogFile : boost::noncopyable
{
 public:
  LogFile(const string& basename,
          size_t rollSize,
          bool threadSafe = true,   //�̰߳�ȫ��Ĭ��Ϊtrue
          int flushInterval = 3,    //ˢ�¼��Ĭ��3s
          int checkEveryN = 1024);  //ÿд1024�μ��һ�Σ��Ƿ���Ҫ��־����
  ~LogFile();

  void append(const char* logline, int len);
  void flush();
  bool rollFile();

 private:
  void append_unlocked(const char* logline, int len);

  static string getLogFileName(const string& basename, time_t* now);

  const string basename_;		//��־�ļ���basename
  const size_t rollSize_;		//��־�ļ�д��rollsize�ͻ�һ�����ļ�
  const int flushInterval_;     //��־д��ļ��ʱ��
  const int checkEveryN_;
  
  //����������count_����checkEveryN��ʱ������Ƿ���Ҫ��һ���µ���־�ļ�
  int count_;   

   //����ָ��
  boost::scoped_ptr<MutexLock> mutex_; 
   //��ʼ��¼��־��ʱ�䣬(�������0��ʱ��),Ϊ�˷�����ʱ��Ϊ��λ����־����
  time_t startOfPeriod_;
  time_t lastRoll_;  //��һ�ι�����־��ʱ��
  time_t lastFlush_;   //��һ��д����־��ʱ��

  
  boost::scoped_ptr<FileUtil::AppendFile> file_;

  const static int kRollPerSeconds_ = 60*60*24;  //һ�죬������ʶһ�����һ����־
};

}
#endif  // MUDUO_BASE_LOGFILE_H
