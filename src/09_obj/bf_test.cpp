#include <iostream>
#include <boost/function.hpp>
#include <boost/bind.hpp>
using namespace std;

class Foo {
public:
    // 四个参数，还隐藏了一个this指针
	void memberFunc(double d, int i, int j) {
		cout << d << endl;  //打印0.5
		cout << i << endl;  //打印100       
		cout << j << endl;  //打印10
	}
};
int main()
{
	Foo foo;
    // 相当于 void f(int)， 4个参数的接口适配成1个参数
    // 把一种接口适配成另一种接口
    // _1是个占位符
    boost::function<void (int)> fp = boost::bind(&Foo::memberFunc, &foo, 0.5, _1, 10);
    // 相当于 (&foo)->memberFunc(0.5, 100, 10)
    fp(100);
	boost::function<void (int, int)> fp1 = boost::bind(&Foo::memberFunc, &foo, 0.5, _1, _2);
	fp1(100, 200);
    // boost::ref(foo)表示一个引用
    // 相当于foo.memberFunc(...)
	boost::function<void (int, int)> fp2 = boost::bind(&Foo::memberFunc, boost::ref(foo), 0.5, _1, _2);
	fp2(55, 66);
	return 0;
}
