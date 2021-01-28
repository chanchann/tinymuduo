#ifndef FOO_H
#define FOO_H
#include <iostream>
#include <pthread.h>

template<typename T>
class Foo{
public:
	static T& instance()
  	{
    	if (!t_value_)
		{
			pthread_key_create(&pkey_, &Foo::destory); 
			t_value_ = new T();
			printf("after constructed, t_value_ is %p\n", t_value_);
			assert(pthread_getspecific(pkey_) == NULL);
      		pthread_setspecific(pkey_, t_value_); 
		}
		return *t_value_;
  	}

	static void destory(void* obj)
	{
		if(t_value_ == NULL)
			printf("t_value_ is NULL!!!\n");
		printf("before deleted, t_value_ is %p, obj is %p\n", t_value_, obj); 
		t_value_ = static_cast<T*>(obj);
		delete t_value_;
		printf("t_value_ is deleted!!!\n");
	}
private:	
	static __thread T* t_value_;
	static pthread_key_t pkey_;
};

template<typename T>
__thread T* Foo<T>::t_value_ = 0;     // __thread变量值只能初始化为编译器常量(值在编译器就可以确定)

template<typename T>
pthread_key_t Foo<T>::pkey_ = 0;
#endif
