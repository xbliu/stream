#ifndef _BASIC_THREAD_H_
#define _BASIC_THREAD_H_


typedef void *(*task_func)(void *);


typedef struct {
	char *name;	//thread name
	int  cpuid; //bind which cpu
	int piority;

	void *arg;
	task_func entry;

	uint8_t is_detach;
	uint8_t debug_on; //dump stack
} task_t;


void dump_thread_stack();
int init_task(task_t *task);
task_t *alloc_init_task();
int free_task(task_t *task);
int create_thread(pthread_t *tid, task_t *task);

#endif

