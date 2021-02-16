// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCALSINGLETON_H
#define MUDUO_BASE_THREADLOCALSINGLETON_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace muduo
{

//�õ������ֱ����̴߳洢����
template<typename T>
class ThreadLocalSingleton : boost::noncopyable
{
 public:

  static T& instance()
  {
    if (!t_value_)
    {
	  
	  //ͨ������Deleter�������õ�����ʱ��ֻ��Ϊ�߳�������һ������
      t_value_ = new T();
      deleter_.set(t_value_);    //����ֱ��ʹ��deleter,�ڳ������п�ʼ�Ѿ�Ϊ��̬���ʼ����
    }
    return *t_value_;
  }

  static T* pointer()
  {
    return t_value_;
  }

 private:
  ThreadLocalSingleton();
  ~ThreadLocalSingleton();

  static void destructor(void* obj)
  {
    assert(obj == t_value_);
    typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
    T_must_be_complete_type dummy; (void) dummy;
    delete t_value_;
    t_value_ = 0;
  }

	//���Ƕ����ֻ��Ϊ��ʵ���Զ����ٶ��󣬲���Ҫ������ʽ����
	//���Ը�������Ҫ��װget�����������Ѿ�����pointer����
	//��Deleter���ٵ�ʱ�򣬻���������������̶������destructorʵ�ֶ�����Զ�����
  class Deleter
  {
   public:
    Deleter()
    {
	  //ָ��һ������ʱ�Ļص�����destructor�������Ͳ���Ҫ����ÿ����ʽ�ĵ���ɾ��������     
      pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
    }

    ~Deleter()
    {
      pthread_key_delete(pkey_);
    }

    void set(T* newObj)
    {
      assert(pthread_getspecific(pkey_) == NULL);
      pthread_setspecific(pkey_, newObj);
    }

    pthread_key_t pkey_;
  };

  static __thread T* t_value_;   //Ҳ���߳�˽�е�ָ�룬ָ����POD���ͣ�����ֱ��__thread����
  static Deleter deleter_;    //��̬��Delter�࣬
};


//��̬��Ķ���
template<typename T>
__thread T* ThreadLocalSingleton<T>::t_value_ = 0;

template<typename T>
typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;

}
#endif