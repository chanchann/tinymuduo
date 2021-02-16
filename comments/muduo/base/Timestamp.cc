less_than_comparable#include <muduo/base/Timestamp.h>

#include <sys/time.h>
#include <stdio.h>


//�μ�toString����ʵ��
//PRId64 ��Ϊ�˽��32λ��64λ�ж���int64_t�������%lld��%ld�Ĳ�ͬ
//PRId64�Ǹ�C�����õġ�C++��ʹ��PRId64���Ǿ͵�ʹ��һ���꣺__STDC_FORMAT_MACROS��
//�μ� http://www.leoox.com/?p=229 
//���������֮����Ҫ����inttypes.hͷ�ļ�
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

#include <boost/static_assert.hpp>

using namespace muduo;

//����ʱ���ԣ��������ʱ������������ֱ�ӱ���
//�ж�Timestap�ǲ���64��λ����Ϊ��ֻ��һ��64λ�ĳ�Ա����
BOOST_STATIC_ASSERT(sizeof(Timestamp) == sizeof(int64_t));

string Timestamp::toString() const
{
  char buf[32] = {0};
  int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
  int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
  //�μ��ļ���ͷ����ĺ�
  snprintf(buf, sizeof(buf)-1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
  return buf;
}

string Timestamp::toFormattedString(bool showMicroseconds) const
{
  char buf[32] = {0};
  time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
  struct tm tm_time;
   //��ʱ��ת��Ϊ�ṹ�壬��gmtime_rΪ�̰߳�ȫ�ĺ���
  gmtime_r(&seconds, &tm_time);

  if (showMicroseconds)
  {
    int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
             microseconds);
  }
  else
  {
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
  }
  return buf;
}

Timestamp Timestamp::now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t seconds = tv.tv_sec;
  return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

