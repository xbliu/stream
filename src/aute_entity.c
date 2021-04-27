#include "autel_entity.h"


autel_pad_t *autel_pad_alloc(int num)
{
	autel_pad_t *pad = (autel_pad_t *)calloc(num,sizeof(autel_pad_t));
	if (!pad) {
		fprintf(stderr,"no mem alloc!\n");
		return NULL;
	}

	return pad;
}

int autel_pad_link(autel_pad_t *src_pad, autel_pad_t *sink_pad)
{
	autel_link_t *link = NULL;

	if (!src_pad || !sink_pad) {
		fprintf(stderr,"illegal parameter!\n");
		return -1;
	}

	link = (autel_link_t *)calloc(1,sizeof(autel_link_t));
	if (!link) {
		fprintf(stderr,"no mem alloc!\n");
		return -1;
	}

	link->src_pad = src_pad;
	link->sink_pad = sink_pad;
	list_add(link->list, src_pad->links);

	return 0;
}

int autel_pad_double_link(autel_pad_t *src_pad, autel_pad_t *sink_pad)
{
	int ret = 0;
	
	if (!src_pad || !sink_pad) {
		fprintf(stderr,"illegal parameter!\n");
		return -1;
	}

	ret = autel_pad_link(src_pad,sink_pad);
	if (ret < 0) {
		return -1;
	}

	ret = autel_pad_link(sink_pad,src_pad);
	return ret;
}

int autel_entity_alloc(int num_pads, int own_sink, entity_operations_t *ops)
{
	autel_pad_t *pads = NULL;
	autel_entity_t *entity = NULL;	

	if (own_sink) {
		num_pads += 1;
	}
	
	entity = (autel_entity_t *)malloc(sizeof(autel_entity_t)+num_pads*sizeof(autel_pad_t));
	if (!entity) {
		return -1;
	}

	pads = (autel_pad_t *)(entity + 1);
	if (own_sink) {
		entity->sink_pad = pads;
		pads++;
	}
	entity->num_srcpads = own_sink ? (num_pads -1) : num_pads;
	entity->src_pad = pads;
	entity->ops = ops;
	
	return 0;
}

int autel_entity_set_ops(autel_entity_t *entity, entity_operations_t *ops)
{
	if (!entity) {
		fprintf(stderr,"illegal parameter!\n");
		return -1;
	}

	entity->ops = ops;
	return 0;
}

int autel_pipeline_add_entity(autel_pipeline_t *pipeline, autel_entity_t *entity)
{
	list_add(entity->list, pipeline->entitys);
	pipeline->num_entitys++;

	return 0;
}


