#include "Thread.h"
#include <unistd.h>
#include <iostream>
using namespace std;

// 面向对象风格，会暴露抽象类
class TestThread : public Thread {
public:
	TestThread(int count) : count_(count) {
		cout << "TestThread ..." << endl;
	}

	~TestThread() {
		cout << "~TestThread ..." << endl;
	}

private:
	void Run() {
		while (count_--) {
			cout << "this is a test ..." << endl;
			sleep(1);
		}
	}
	int count_;
};

int main(void) {
    /*
	TestThread t(5);
	t.Start();
	t.Join();       
    */

    // t.Run(); // 虽然结果一样，但这就在主线程中运行了

	TestThread* t2 = new TestThread(5);
	t2->SetAutoDelete(true);
	t2->Start();
	t2->Join();

	for (; ; )
		pause();

	return 0;
}