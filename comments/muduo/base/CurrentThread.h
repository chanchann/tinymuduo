// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include <stdint.h>

namespace muduo
{

//CurrentThread���ڱ����߳�˽������
//��Щ���Ǿ�̬����������ֱ�ӵ������� muduo::CurrentThread::tid()
namespace CurrentThread
{
  //extern �ؼ��ֱ�ʶ����Щ������Ҫ�������ļ��ж��� 
  //��������ռ䱾������Thread.cc��ʵ�ֵģ�������Thrad.cc���ӣ�����ȡ�����ˣ�������Ҫ����extern

  // internal
  extern __thread int t_cachedTid;
  extern __thread char t_tidString[32];
  extern __thread int t_tidStringLength;
  extern __thread const char* t_threadName;
  void cacheTid();

  inline int tid()
  {
	  //__builtin_expect��gccΪ�Ż�cpuִ��ָ�
	  //�������൱���ж�t_cachedTid�Ƿ����0
    if (__builtin_expect(t_cachedTid == 0, 0))
    {
      cacheTid();
    }
    return t_cachedTid;
  }

  inline const char* tidString() // for logging
  {
    return t_tidString;
  }

  inline int tidStringLength() // for logging
  {
    return t_tidStringLength;
  }

  inline const char* name()
  {
    return t_threadName;
  }

  bool isMainThread();

  void sleepUsec(int64_t usec);
}
}

#endif
