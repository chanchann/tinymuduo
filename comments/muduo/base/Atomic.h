// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_ATOMIC_H
#define MUDUO_BASE_ATOMIC_H

#include <boost/noncopyable.hpp>
#include <stdint.h>

/*
*
gccԭ���Բ���:
// ԭ����������
type __sync_fetch_and_add (type *ptr, type value)

// ԭ�ӱȽϺͽ��������ã�����  (����Ƚ���Ⱦ�����Ϊnewval)
type __sync_val_compare_and_swap (type *ptr, type oldval type newval)
bool __sync_bool_compare_and_swap (type *ptr, type oldval type newval)

// ԭ�Ӹ�ֵ����
type __sync_lock_test_and_set (type *ptr, type value)
ʹ����Щԭ���Բ����������ʱ����Ҫ��-march=cpu-type   
����ֱ��д��-match=native�� ��CPU�Լ��������ʲô��
*/

namespace muduo
{

namespace detail
{

//Ϊ���ɿ������࣬boost::noncopyableʵ�־��ǽ��������캯���͸������������˽�еġ�
template<typename T>
class AtomicIntegerT : boost::noncopyable
{
 public:
 	//��ʼ��0������ֱ������
  AtomicIntegerT()
    : value_(0)
  {
  }

  // uncomment if you need copying and assignment
  //
  // AtomicIntegerT(const AtomicIntegerT& that)
  //   : value_(that.get())
  // {}
  //
  // AtomicIntegerT& operator=(const AtomicIntegerT& that)
  // {
  //   getAndSet(that.get());
  //   return *this;
  // }

  T get()
  {
    // in gcc >= 4.7: __atomic_load_n(&value_, __ATOMIC_SEQ_CST)
    return __sync_val_compare_and_swap(&value_, 0, 0);
  }

  T getAndAdd(T x)
  {
    // in gcc >= 4.7: __atomic_fetch_add(&value_, x, __ATOMIC_SEQ_CST)
    return __sync_fetch_and_add(&value_, x);
  }

  T addAndGet(T x)
  {
    return getAndAdd(x) + x;
  }

  T incrementAndGet()
  {
    return addAndGet(1);
  }

  T decrementAndGet()
  {
    return addAndGet(-1);
  }

  void add(T x)
  {
    getAndAdd(x);
  }

  void increment()
  {
    incrementAndGet();
  }

  void decrement()
  {
    decrementAndGet();
  }

  T getAndSet(T newValue)
  {
    // in gcc >= 4.7: __atomic_exchange_n(&value, newValue, __ATOMIC_SEQ_CST)
    return __sync_lock_test_and_set(&value_, newValue);
  }

 private:
 	//��volatile���Σ���ֹ�����������Ż�����֤ÿ��ȡֵ����ֱ�Ӵ��ڴ���ȡֵ
  volatile T value_;
};
}

typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
typedef detail::AtomicIntegerT<int64_t> AtomicInt64;
}

#endif  // MUDUO_BASE_ATOMIC_H
