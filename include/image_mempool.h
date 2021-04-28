#ifndef _IMAGE_MEM_POOL_H_
#define _IMAGE_MEM_POOL_H_

#include <stdint.h>
#include "vision_service.h"

#define	IMAGE_MEMPOOL_MAGIC (0x5aa53cc3)
#define MEM_DATA_ALIGN 		(8)
#define VISION_RESERVE_FRMS (5)


#define IMAGE_SHM_BASEADDR	(IMU_SHM_BASEADDR + IMU_SHM_MAXSIZE)
#define IMAGE_SHM_MAXSIZE	(0x40000)
#define SHM_POOL_BASEADDR   (IMAGE_SHM_BASEADDR + IMAGE_SHM_MAXSIZE)
#define SHM_POOL_SIZE       (0x40000)

//will need to move to another location
#define DDR_MAPSIZE (0x1000)
#define DATA_ALIGN(len, size) (((len) + (size -1))&~((size) -1))


typedef void (*item_init_func)(void *handle, int index, void *addr, int len);


typedef enum {
    RectImage = 0,
    MatchImage,
    CustomRectImage,
    VisionPoolCount,
    TrackImage = VisionPoolCount,
    DetectImage,
    PoolCount
} image_pool_type_e;


typedef struct {
	uint32_t pool_magic;
	uint32_t calib_status;	
	uint32_t camera_status;
	uint32_t vision_status;
	calib_info_t calib_info;
	uint32_t extend_status;
} image_mempool_info_t;

#define IMAGE_POOL_NUM (VisionPoolCount * VISION_STREAM_BUTT + PoolCount - VisionPoolCount)

typedef struct {
    int pool_size;
    char* vaddr; //virtual addr

	image_mempool_info_t* info;
	char* addr[IMAGE_POOL_NUM];
    mem_pool_t *image[IMAGE_POOL_NUM];
} image_mempool_t;

typedef struct {
    uint32_t type;
    uint32_t item_size;
    uint32_t item_count;
    item_init_func item_init;
} image_item_pool_t;


#endif

