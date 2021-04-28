#include "image_mempool.h"
#include "mempool.h"

#define GET_IMAGE_LENGTH_FUNC(name,type) \
	void get_##name##_image_length() \
	{ \
		return sizeof(type); \
	}

#define IMAGE_INIT_FUNC(name,type,ops)	\
	void name##_image_init(void *handle, int stream, void *addr, int len) \
	{ \
		mem_pool_t *pool = NULL; \
		pool = get_mempool(handle, stream, type); \
		return mempool_init(pool, addr, len, ops); \
	}

#define ELEM_OPERATIONS_DEFINE(name) \
	elem_operations_t name##_image_ops = { \
		.get_length = get_##name##_image_length, \
	}


static image_item_pool_t image_item[PoolCount] = {
    {RectImage,          sizeof(RectFrame),         VISION_RESERVE_FRMS,     rect_image_init},
    {MatchImage,         sizeof(MatchFrame),   		VISION_RESERVE_FRMS,     match_image_init},
    {CustomRectImage,	 sizeof(CustomRectFrame),	VISION_RESERVE_FRMS,	 NULL},
    {TrackImage,		 sizeof(VpssFrame),			1,	   					 track_image_init},
    {DetectImage,	     sizeof(VpssFrame),			1,	   					 detect_image_init},
};


static mem_pool_t *get_mempool(void *handle, int stream, image_pool_type_e pool_type)
{
	int pool_index = 0;
	image_mempool_t* pool = (image_mempool_t*)handle;

    assert(pool);

	if (pool_type < VisionPoolCount) {
		pool_index = VISION_STREAM_BUTT * pool_type + stream;
	} else {
		pool_index = VISION_STREAM_BUTT * VisionPoolCount + pool_type - VisionPoolCount;
	}
    
    return pool->image[pool_index];
}

static int get_image_item_pool_size(image_item_pool_t *pool)
{
	int i = 0;
	int j = 0;
	int pool_size = 0;

	for(i = 0; i < VisionPoolCount; i++){
		for(j = 0; j < VISION_STREAM_BUTT; j++){
        	pool_size += DATA_ALIGN(pool[i].item_count * pool[i].item_size, MEM_DATA_ALIGN);
			pool_size += DATA_ALIGN(sizeof(mem_pool_t), MEM_DATA_ALIGN);
		}
    }

	for(i = VisionPoolCount; i < PoolCount; i++){
		pool_size += DATA_ALIGN(pool[i].item_count * pool[i].item_size, MEM_DATA_ALIGN);
		pool_size += DATA_ALIGN(sizeof(mem_pool_t), MEM_DATA_ALIGN);
	}

	return pool_size;
}


static ELEM_OPERATIONS_DEFINE(rect);
static ELEM_OPERATIONS_DEFINE(match);
static ELEM_OPERATIONS_DEFINE(custom_rect);
static ELEM_OPERATIONS_DEFINE(track);
static ELEM_OPERATIONS_DEFINE(detect);

static GET_IMAGE_LENGTH_FUNC(rect,RectFrame)
static GET_IMAGE_LENGTH_FUNC(match,MatchFrame)
static GET_IMAGE_LENGTH_FUNC(custom_rect,CustomRectFrame)
static GET_IMAGE_LENGTH_FUNC(track,VpssFrame)
static GET_IMAGE_LENGTH_FUNC(detect,VpssFrame)

IMAGE_INIT_FUNC(rect,RectImage,rect_image_ops)
IMAGE_INIT_FUNC(match,MatchImage,match_image_ops)
IMAGE_INIT_FUNC(custom_rect,CustomRectImage,custom_rect_image_ops)
IMAGE_INIT_FUNC(track,TrackImage,track_image_ops)
IMAGE_INIT_FUNC(detect,DetectImage,detect_image_ops)


uint32_t* image_mempool_get_calib_status(void *handle)
{
	image_mempool_t* pool = (image_mempool_t*)handle;

	if (NULL != pool && pool->info->pool_magic == IMAGE_MEMPOOL_MAGIC) {
		return &pool->info->calib_status;
	}

	return NULL;
}

uint32_t* image_mempool_get_camera_status(void * handle)
{
	image_mempool_t* pool = (image_mempool_t*)handle;

	if (NULL != pool && pool->info->pool_magic == IMAGE_MEMPOOL_MAGIC) {
		return &pool->info->camera_status;
	}

	return NULL;
}

uint32_t* image_mempool_get_vision_status(void * handle)
{
	image_mempool_t* pool = (image_mempool_t*)handle;

	if (NULL != pool && pool->info->pool_magic == IMAGE_MEMPOOL_MAGIC) {
		return &pool->info->vision_status;
	}

	return NULL;
}

void* image_mempool_get_calib_info(void * handle)
{
	image_mempool_t* pool = (image_mempool_t*)handle;

	if (NULL != pool) {
		return &pool->info->calib_info;
	}

	return NULL;
}

uint32_t* image_mempool_get_extend_status(void * handle)
{
	image_mempool_t* pool = (image_mempool_t*)handle;

	if (NULL != pool) {
		return &pool->info->extend_status;
	}

	return NULL;
}

void * image_mempool_init(int readonly, int* is_server)
{
	int i = 0;
	int j = 0;
	int index = 0;
	int offset = 0;
	int pool_size = 0;
	uint32_t saved_status = 0;
	char *ptr = NULL;
	int info_end_offset = 0;

    image_mempool_t* image_pool = (image_mempool_t*)calloc(1, sizeof(image_mempool_t));
	if (!image_pool) {
		printf("no mem alloc!\n");
		return NULL;
	}

	if(0 == readonly){
		if(!is_server){
			free(image_pool);
			return NULL;
		}
		*is_server = 0;
	}

	pool_size += DATA_ALIGN(sizeof(image_mempool_info_t), MEM_DATA_ALIGN);
	info_end_offset = pool_size;
	pool_size += get_image_item_pool_size(image_item);
	
    pool_size = DATA_ALIGN(pool_size, DDR_MAPSIZE);
	if(pool_size > IMAGE_SHM_MAXSIZE){
		printf("Memory size reserved for image is not enough!\n");
		goto err_out;
	}
    image_pool->vaddr = (char*)common_memmap(IMAGE_SHM_BASEADDR, pool_size);
    if(!image_pool->vaddr){
        goto err_out;
    }

    image_pool->pool_size = pool_size;
    image_pool->info = (image_mempool_info_t*)image_pool->vaddr;
	if(0 == readonly && image_pool->info->pool_magic != IMAGE_MEMPOOL_MAGIC){
		*is_server = 1;
		image_pool->info->pool_magic = IMAGE_MEMPOOL_MAGIC;
		image_pool->info->vision_status = VISION_STATUS_SAVEBITS;
		image_pool->info->calib_status = 0;
		image_pool->info->camera_status = 0xFFF00000;
	}

	offset = info_end_offset;
    for(i = 0; i < VisionPoolCount; i++){
		for(j = 0; j < VISION_STREAM_BUTT; j++){
        	image_pool->addr[VISION_STREAM_BUTT * i + j] = (char*)(image_pool->vaddr + offset);
        	offset += DATA_ALIGN(image_item[i].item_count * image_item[i].item_size, MEM_DATA_ALIGN);
		}
    }
	for(i = VisionPoolCount; i < PoolCount; i++){
		index = VISION_STREAM_BUTT*VisionPoolCount + i - VisionPoolCount;
		image_pool->addr[index] = (char*)(image_pool->vaddr + offset);
        offset += DATA_ALIGN(image_item[i].item_count * image_item[i].item_size, MEM_DATA_ALIGN);
	}

    for(i = 0; i < VisionPoolCount; i++){
		for(j = 0; j < VISION_STREAM_BUTT; j++){
	        image_pool->image[VISION_STREAM_BUTT * i + j] = (mem_pool_t*)(image_pool->vaddr + offset);
			if(0 == readonly && *is_server && image_item[i].item_init){
	            image_item[i].item_init(image_pool, j, image_pool->addr[VISION_STREAM_BUTT * i + j], image_item[i].item_count);
	        }
	        offset += DATA_ALIGN(sizeof(mem_pool_t), MEM_DATA_ALIGN);
		}
    }

	for(i = VisionPoolCount; i < PoolCount; i++){
		index = VISION_STREAM_BUTT * VisionPoolCount + i - VisionPoolCount;
		image_pool->image[index] = (mem_pool_t*)(image_pool->vaddr + offset);
		if(0 == readonly && *is_server && image_item[i].item_init){
            image_item[i].item_init(image_pool, 0, image_pool->addr[index], image_item[i].item_count);
        }
        offset += DATA_ALIGN(sizeof(mem_pool_t), MEM_DATA_ALIGN);
	}

    return (void *)image_pool;

err_out:
    free(image_pool);
    return NULL;
}

int image_mempool_deinit(void * handle)
{
	image_mempool_t* image_pool = (image_mempool_t*)handle;
    if(!image_pool){
        return -1;
    }
    if(image_pool->vaddr){
        common_memunmap((void*)(image_pool->vaddr),image_pool->pool_size);
    }
    free(image_pool);

    return 0;
}

//need to modify
void * image_mempool_custom_rect_init(void)
{
	int i = 0, j, pool_size = 0, offset = 0, index;
    image_mempool_t* image_pool = (image_mempool_t*)calloc(1, sizeof(image_mempool_t));
    assert(image_pool);
    void * handle = (void *)image_pool;
	pool_size += DATA_ALIGN(sizeof(image_mempool_info_t), MEM_DATA_ALIGN);
	offset = pool_size;
    for(i = 0; i < VisionPoolCount; i++){
		for(j = 0; j < VISION_STREAM_BUTT; j++){
        	pool_size += DATA_ALIGN(image_item[i].item_count * image_item[i].item_size+sizeof(mem_pool_t), MEM_DATA_ALIGN);
		}
    }
	for(i = VisionPoolCount; i < PoolCount; i++){
		pool_size += DATA_ALIGN(image_item[i].item_count * image_item[i].item_size+sizeof(mem_pool_t), MEM_DATA_ALIGN);
	}
    pool_size = DATA_ALIGN(pool_size, DDR_MAPSIZE);
    image_pool->vaddr = (char*)common_memmap(IMAGE_SHM_BASEADDR, pool_size);
    if(!image_pool->vaddr){
        goto FAILED_OUT;
    }
    image_pool->vaddr = image_pool->vaddr;
    image_pool->pool_size = pool_size;
    image_pool->info = (image_mempool_info_t*)image_pool->vaddr;

    for(i = 0; i < VisionPoolCount; i++){
		for(j = 0; j < VISION_STREAM_BUTT; j++){
        	image_pool->addr[VISION_STREAM_BUTT * i + j] = (char*)(image_pool->vaddr + offset);

        	offset += DATA_ALIGN(image_item[i].item_count * image_item[i].item_size, MEM_DATA_ALIGN);
		}
    }
	for(i = VisionPoolCount; i < PoolCount; i++){
		index = VISION_STREAM_BUTT*VisionPoolCount + i - VisionPoolCount;
		image_pool->addr[index] = (char*)(image_pool->vaddr + offset);

        offset += DATA_ALIGN(image_item[i].item_count * image_item[i].item_size, MEM_DATA_ALIGN);
	}

    for(i = 0; i < VisionPoolCount; i++){
		for(j = 0;j < VISION_STREAM_BUTT; j++){
			index = VISION_STREAM_BUTT * i + j;
	        image_pool->image[index] = (mem_pool_t*)(image_pool->vaddr + offset);
			if(i == CustomRectImage){
	        	custom_rect_image_init(handle, j, image_pool->addr[index], image_item[i].item_count);
			}
	        offset += DATA_ALIGN(sizeof(mem_pool_t),MEM_DATA_ALIGN);
		}
    }
	for(i = VisionPoolCount; i < PoolCount; i++){
		index = VISION_STREAM_BUTT * VisionPoolCount + i - VisionPoolCount;
		image_pool->image[index] = (mem_pool_t*)(image_pool->vaddr + offset);
        offset += DATA_ALIGN(sizeof(mem_pool_t), MEM_DATA_ALIGN);
	}

    return handle;
FAILED_OUT:
    free(image_pool);
    return NULL;
}


void image_mempool_insert(void *handle, int type, int stream, void *elem)
{
	mem_pool_t *pool = NULL;
	pool = get_mempool(handle, stream, type);
	return mempool_insert(pool, elem);
}

int image_mempool_peek(void *handle, int type, int stream, void *elem, pool_context_t *ctx)
{
	mem_pool_t *pool = NULL;
	pool = get_mempool(handle, stream, type);
	return mempool_peek(pool, elem, ctx);
}

int image_mempool_peek_index(void *handle, int type, int stream, void *elem, int index)
{
	mem_pool_t *pool = NULL;
	pool = get_mempool(handle, stream, type);
	return mempool_peek_index(pool, elem, index);
}

