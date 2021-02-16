// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_SINGLETON_H
#define MUDUO_BASE_SINGLETON_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <stdlib.h> // atexit
#include <pthread.h>

namespace muduo
{

namespace detail
{
// This doesn't detect inherited member functions!
// http://stackoverflow.com/questions/1966362/sfinae-to-check-for-inherited-member-functions
template<typename T>
struct has_no_destroy
{
//todo:what does it mean?
#ifdef __GXX_EXPERIMENTAL_CXX0X__
  template <typename C> static char test(decltype(&C::no_destroy));
#else
  template <typename C> static char test(typeof(&C::no_destroy));
#endif
  template <typename C> static int32_t test(...);
  const static bool value = sizeof(test<T>(0)) == 1;
};
}

template<typename T>
class Singleton : boost::noncopyable  //�̰߳�ȫ�ĵ�����ʵ��
{
 public:
  static T& instance()
  {
    //pthread_once�ܹ���֤init����ֻ��������һ�Σ�Ҳ���Ƕ���ֻ��������һ��
    //�ú������̰߳�ȫ��
    pthread_once(&ponce_, &Singleton::init);
    assert(value_ != NULL);
    return *value_;
  }

 private:
  Singleton();
  ~Singleton();

  static void init()
  {
    value_ = new T();
    if (!detail::has_no_destroy<T>::value)
    {
      ::atexit(destroy);  //ע���˳�ʱ���ٺ���
    }
  }

  static void destroy()
  {
    //sizeof���ڱ���ʱ�ͼ����T�����ʹ�С������ǲ���ȫ���ͣ�sizeof(T)Ϊ0, 
    //T_must_be_complete_type���൱�ڸ���Ϊ-1��Ԫ�ص����飬���������ʱ��ᱨ��
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
	//����������У���dummy���������壬���Ƕ����ûʹ�ã�Ϊ��ֹ�������Լ���(void) dummy;
    T_must_be_complete_type dummy; (void) dummy;

    delete value_;
    value_ = NULL;
  }

 private:
  static pthread_once_t ponce_;
  static T*             value_;
};

template<typename T>
pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

template<typename T>
T* Singleton<T>::value_ = NULL;

}
#endif

