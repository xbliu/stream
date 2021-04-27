#include <autel_entity.h>


typedef struct {
	autel_entity_t entity;
	char *file_name;
	uint32_t is_running;
	pthread_t thd_id;
	task_t *task;
} file_src_t;

static file_src_t file_src;


int file_src_set_filename(char *file_name)
{
	file_src.file_name = file_name;
	return 0;
}

static int file_src_start(autel_entity_t *entity, void *param)
{
	file_src_t *file_src = NULL;

	file_src = container_of(entity,file_src_t,entity);
	file_src->is_running = 1;
	file_src->task = alloc_init_task();
	if (!file_src->task) {
		return -1;
	}

	return create_thread(file_src->thd_id,file_src->task);
}

static int file_src_stop(autel_entity_t *entity, void *param)
{
	file_src_t *file_src = NULL;

	file_src = container_of(entity,file_src_t,entity);
	file_src->is_running = 0;
	usleep(1000000); //need to modify
	free_task(file_src->task);

	return 0;
}



