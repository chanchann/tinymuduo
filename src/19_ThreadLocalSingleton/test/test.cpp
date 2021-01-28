#include <iostream>
#include "Foo.h"
#include <pthread.h>

using namespace std;
class Test
{
 public:
  Test()
  {
    printf("constructing\n");
  }

  ~Test()
  {
    printf("destructing\n");
  }
};

void* threadFunc(void* arg)
{
  Foo<Test>::instance();
	// Foo<Test>::destory();
  return NULL;  
}

int main(int argc, char* argv[])
{
  pthread_t tid;
  pthread_create(&tid, NULL, threadFunc, NULL);
  pthread_join(tid, NULL);

	return 0;
}

