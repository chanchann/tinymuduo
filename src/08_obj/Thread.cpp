/*
主要演示线程类的封装
*/
#include "Thread.h"
#include <iostream>
using namespace std;

Thread::Thread() : autoDelete_(false) {
	cout << "Thread ..." << endl;
}

Thread::~Thread() {
	cout << "~Thread ..." << endl;
}

void Thread::Start() {
    // 这里的回调函数不可以直接传Run()
    // Run是普通的成员函数，隐含的第一个参数是Thread*(this)
    // 调用的时候是this call约定
    // 可以通过一个全局函数解决，但我们不希望暴露出去
    // 所以我们 static void* ThreadRoutine(void* arg);
    // 传this -> 传当前对象自身，这里的this指向的一定是派生类
    pthread_create(&threadId_, NULL, ThreadRoutine, this);
}

void Thread::Join() {
	pthread_join(threadId_, NULL);
}

void* Thread::ThreadRoutine(void* arg) {
    // 这里不可以直接调用，因为静态成员函数不能调用非静态的成员函数
    // 因为静态成员函数没有this指针
    // Run();  

    // 基类指针指向的派生类对象
    // 所以运行的是派生类实现的Run()
    // 这里利用了虚函数的多态 
	Thread* thread = static_cast<Thread*>(arg);
	thread->Run();
	if (thread->autoDelete_)
		delete thread;
	return NULL;
}

// 线程对象的生命周期 和 线程的生命周期不一样
// 我们需要实现， 线程执行完毕，线程对象能够自动销毁
void Thread::SetAutoDelete(bool autoDelete) {
	autoDelete_ = autoDelete;
}