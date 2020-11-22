#include "Thread.h"
#include <boost/bind.hpp>
#include <unistd.h>
#include <iostream>
using namespace std;

class Foo {
public:
	Foo(int count) : count_(count) {}

	void MemberFun() {
		while (count_--) {
			cout << "this is a test ... : " << count_ <<endl;
			sleep(1);
		}
	}

	void MemberFun2(int x) {
		while (count_--) {
			cout << "x=" << x << " this is a test2 ... : " << count_ << endl;
			sleep(1);
		}
	}

	int count_;
};

void ThreadFunc() {
	cout<<"ThreadFunc ..."<<endl;
}

// 不符合参数所以要用boost bind做个接口转化
void ThreadFunc2(int count) {
	while (count--) {
		cout<< "ThreadFunc2 ... : " << count <<endl;
		sleep(1);
	}
}


int main(void) {
	Thread t1(ThreadFunc);
	Thread t2(boost::bind(ThreadFunc2, 3));
	Foo foo(3);
    // 成员函数一定要注意&
	Thread t3(boost::bind(&Foo::MemberFun, &foo));
	Foo foo2(3);
	Thread t4(boost::bind(&Foo::MemberFun2, &foo2, 1000));

	t1.Start();
	t2.Start();
	t3.Start();
	t4.Start();

	t1.Join();
	t2.Join();
	t3.Join();
	t4.Join();
	return 0;
}
