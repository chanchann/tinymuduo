// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>

namespace muduo
{

//�ȿ��������������̵߳ȴ����̷߳��� �����ܡ� 
//Ҳ�����������̵߳ȴ����̳߳�ʼ����ϲſ�ʼ����
//ʹ�ð����μ�gaorongTest�´���
class CountDownLatch : boost::noncopyable
{
 public:

  explicit CountDownLatch(int count);

  void wait();

  void countDown();

  int getCount() const;

 private:
 //��Ϊ������getCount��const����,������Ҫ�ڸú������޸�mutex_���Խ��䶨��Ϊmutable
  mutable MutexLock mutex_;
  Condition condition_;
  int count_;
};

}
#endif  // MUDUO_BASE_COUNTDOWNLATCH_H
