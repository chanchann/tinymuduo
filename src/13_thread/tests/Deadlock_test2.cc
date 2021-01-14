#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

// 子进程复制父进程的数据段，BSS段，代码段，堆空间，栈空间，文件描述符，但是对于文件描述符关联的内核文件表项（即struct file结构体）则是采用共享的方式

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* doit(void* arg)
{
	printf("pid = %d begin doit ...\n",static_cast<int>(getpid()));
	pthread_mutex_lock(&mutex);
	struct timespec ts = {2, 0};
	nanosleep(&ts, NULL);
	pthread_mutex_unlock(&mutex);
	printf("pid = %d end doit ...\n",static_cast<int>(getpid()));

	return NULL;
}

void prepare(void)
{
	pthread_mutex_unlock(&mutex);
}

void parent(void)
{
	pthread_mutex_lock(&mutex);
}

int main(void)
{
	pthread_atfork(prepare, parent, NULL);    // 在fork前会调用prepare()解锁，这样子进程复制之后就可以成功加锁
	printf("pid = %d Entering main ...\n", static_cast<int>(getpid()));
	pthread_t tid;
	pthread_create(&tid, NULL, doit, NULL);
	struct timespec ts = {1, 0};
	nanosleep(&ts, NULL);
	if (fork() == 0) // fork完后，父进程又调用parent()进行加锁，这样子线程可以正常解锁
	{
		doit(NULL);
	}
	pthread_join(tid, NULL);
	printf("pid = %d Exiting main ...\n",static_cast<int>(getpid()));

	return 0;
}
