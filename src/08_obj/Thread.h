#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>
/*
未考虑错误处理
*/
class Thread {
public:
	Thread();
	virtual ~Thread();  // 因为多态，这里必须是虚析构函数

	void Start();
	void Join();

	void SetAutoDelete(bool autoDelete);

private:
    // 加了static 就没有this指针了
	static void* ThreadRoutine(void* arg);
    // 外界不能直接访问
    //设置成纯虚函数，继承需要去实现,其本身不需要去实现
    // 我们需要实现的线程的类的真正的线程执行体
	virtual void Run() = 0;
	pthread_t threadId_;
	bool autoDelete_;  
};

#endif // _THREAD_H_