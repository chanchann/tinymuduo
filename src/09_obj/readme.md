## boost bind/function 库

函数适配器

一种接口 --> 另一种接口

已经是 c++11标准

## 我们利用基于对象编程方法，不用抽象类，只基于具体类

C编程风格：注册三个全局函数到网络库，网络库函数的参数有函数指针类型，里面通过函数指针来回调。

面向对象风格：用一个EchoServer继承自TcpServer（抽象类），实现三个纯虚函数接口OnConnection, OnMessage, OnClose。通过基类指针调用虚函数实现多态。

基于对象风格：用一个EchoServer包含一个TcpServer（具体类）对象成员server，在构造函数中用boost::bind 来注册三个成员函数，如server.SetConnectionCallback(boost::bind(&EchoServer::OnConnection, ...)); 也就是设置了server.ConnectionCallback_ 成员，通过绑定不同的函数指针，调用server.ConnectionCallback_()  时就实现了行为的不同。

https://blog.csdn.net/daaikuaichuan/article/details/85945208