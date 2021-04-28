#ifndef _MEM_POOL_H_
#define _MEM_POOL_H_

#include <stdint.h>


#define MEM_POOL_SUCCESS 				(0)
#define MEM_POOL_FAIL    				(-1)
#define MEM_POOL_NOT_INIT				(-2)
#define MEM_POOL_PARAMS_ILLEGAL			(-3)
#define MEM_POOL_BUSY					(-4)
#define MEM_POOL_NODATA					(-5)

#define	MEM_POOL_MAGIC					(0x3cc3)


typedef struct {
	int (*get_length)();
//	int (*copy)(void *src, void *dst);
} elem_operations_t;

typedef struct {
	uint32_t magic;    
	uint32_t batch;
	int32_t index;
    uint32_t length;
	void *addr;
    uint32_t operating;

	elem_operations_t *ops;
} mem_pool_t;

typedef struct {
	uint32_t batch;
	int32_t index;
} pool_context_t;


void mempool_reset(mem_pool_t *pool);
void mempool_insert(mem_pool_t *pool, void*elem);
void mempool_init(mem_pool_t *pool, void *addr, int len, elem_operations_t *ops);
int mempool_peek(mem_pool_t *pool, void *elem, pool_context_t *ctx);
int mempool_peek_index(mem_pool_t *pool,  void *elem, int index);


#endif

