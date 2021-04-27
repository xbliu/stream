#include "basic_thread.h"

#define MAX_BACKTRACE_NUM (32)
#define MAX_THREAD_NAME_LEN (32)

static int set_pthread_attr(pthread_attr_t *attr, int piority, int  cpuid)
{
	struct sched_param param;
	cpu_set_t cpu_mask;
	
	if (pthread_attr_init(attr)) {
		fprintf(stderr,"pthread attr init err %d: %s \n", errno, strerror(errno));
		return -1;
	}

	if (pthread_attr_setschedpolicy(attr, SCHED_RR)) {
		goto err_destroy;
	}

	if (0 != piority) {
		param.sched_priority = piority;
		if (pthread_attr_setschedparam(attr, &param)) {
			fprintf(stderr,"pthread attr setschedparam err %d: %s \n", errno, strerror(errno));
			goto err_destroy;
		}
	}

	CPU_ZERO(&cpu_mask);
	CPU_SET(cpuid, &cpu_mask);
	if (pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), &cpu_mask)) {
		fprintf(stderr,"pthread attr setaffinity np err %d: %s \n", errno, strerror(errno));
		goto err_destroy;
	}

	if (pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED)) {
		fprintf(stderr,"pthread attr setinheritsched err %d: %s \n", errno, strerror(errno));
		goto err_destroy;
	}

err_destroy:
	pthread_attr_destroy(attr);
	return -1;
}

static void signal_handler(int signum)
{
	char thread_name[MAX_THREAD_NAME_LEN] = {0};
	pthread_getname_np(pthread_self(), thread_name, MAX_THREAD_NAME_LEN);

	fprintf(stderr,"thread %s catch a signal %d and may be crashed!\n", thread_name, signum);

	if ((SIGSEGV == signum) || (SIGABRT == signum)) {
		dump_thread_stack();
	}
}

static void *single_task(void *arg)
{
	task_t *task = (task_t *)arg;

	signal(SIGSEGV, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGBUS, signal_handler);
	signal(SIGFPE, signal_handler);
	signal(SIGSYS, signal_handler);

	return task->entry(task->arg);
}


void dump_thread_stack()
{
	int i = 0;
	int size = 0;
    void *buffer[MAX_BACKTRACE_NUM] = {0};
	char **symbol = NULL;
	char thread_name[MAX_THREAD_NAME_LEN] = {0};

	pthread_getname_np(pthread_self(), thread_name, MAX_THREAD_NAME_LEN);
    size = backtrace(buffer, MAX_BACKTRACE_NUM);
    symbol = backtrace_symbols(buffer, size);
	
    fprintf(stderr,"[%15s]Seg-fault backtrack:\n",thread_name);
    for (i = 0; i < size; ++i) {
      fprintf(stderr,"[%15s][%3d/%3d]: %s\n", thread_name, i, size, symbol[i]);
    }
    free(symbol);
}

int init_task(task_t *task)
{
	memset(task,0,sizeof(task_t));
	task->cpuid = 0xFF;

	return 0;
}

task_t *alloc_init_task()
{
	task_t *task = (task_t *)malloc(sizeof(task_t));
	if (!task) {
		fprintf(stderr,"no mem for task!\n");
		return NULL;
	}

	init_task(task);
	return task;
}

int free_task(task_t *task)
{
	if (!task) {
		free(task);
	}

	return 0;
}

int create_thread(pthread_t *tid, task_t *task)
{
	int ret = 0;
	pthread_attr_t attr;

	ret = set_pthread_attr(&attr,task->piority,task->cpuid);
	if (ret < 0) {
		goto err_set;
	}
	
	if (task->debug_on) {
		ret = pthread_create(tid,&attr,task->entry,task->arg);
	} else {
		ret = pthread_create(tid,&attr,single_task,task);
	}

	if (ret < 0) {
		ret = -1;
		fprintf(stderr,"failed to create thread!\n");
		goto err_create;
	}

	if(!task->name) {
		pthread_setname_np(*tid, task->name);
	}

	if (task->is_detach) {
		pthread_detach(*tid);
	}

err_create:
	pthread_attr_destroy(&attr);
err_set:
	return ret;
}

