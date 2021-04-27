#ifndef _AUTEL_ENTITY_H_
#define _AUTEL_ENTITY_H_

#include <stdint.h>
#include "list.h"

#define AUTEL_OK (0)
#define AUTEL_ERROR (-1)

#ifndef offsetof
#define offsetof(type, member) ((unsigned int) &((type *)0)->member)
#endif

#ifndef container_of
#define container_of(ptr, type, member)  ((type *)( (char *)(ptr) - offsetof(type,member) ))
#endif


typedef struct list_head list_head_t;
typedef struct autel_pad autel_pad_t;
typedef struct autel_entity autel_entity_t;


typedef int (*push_func)(autel_pad_t *src_pad, autel_pad_t *sink_pad, void *buffer);
typedef int (*entity_func)(autel_entity_t *entity, void *param);


typedef struct {
	entity_func start;
	entity_func stop;
} entity_operations_t;

struct autel_pad {
	uint32_t dir;
	uint16_t index;
	autel_entity_t *entity;

	list_head_t links;
	push_func push;
};

struct autel_entity {
	int num_srcpads;
	autel_pad_t *src_pad;
	autel_pad_t *sink_pad;

	list_head_t list;
	entity_operations_t *ops;
};

typedef struct {
	autel_pad_t *src_pad;
	autel_pad_t *sink_pad;

	list_head_t list;
} autel_link_t;

typedef struct {
	uint32_t num_entitys;
	list_head_t entitys;
} autel_pipeline_t;



#endif

