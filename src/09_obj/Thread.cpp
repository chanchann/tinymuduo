#include "Thread.h"
#include <iostream>
using namespace std;

// 按照声明的顺序初始化
Thread::Thread(const ThreadFunc& func) : func_(func), autoDelete_(false) {}

void Thread::Start() {
	pthread_create(&threadId_, NULL, ThreadRoutine, this);
}

void Thread::Join() {
	pthread_join(threadId_, NULL);
}

void* Thread::ThreadRoutine(void* arg) {
	Thread* thread = static_cast<Thread*>(arg);
	thread->Run();
	if (thread->autoDelete_)
		delete thread;
	return NULL;
}

void Thread::SetAutoDelete(bool autoDelete) {
	autoDelete_ = autoDelete;
}

void Thread::Run() {
    // 仅仅是回调了我们的适配函数
	func_();
}