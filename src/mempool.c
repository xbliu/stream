#include "mempool.h"

void mempool_reset(mem_pool_t *pool)
{														
	pool->index = -1;
	pool->batch = 0;
}

void mempool_init(mem_pool_t *pool, void *addr, int len, elem_operations_t *ops)
{
	mempool_reset(pool);
	pool->magic = MEM_POOL_MAGIC;
	pool->addr = addr;
	pool->length = len;
	pool->ops = ops;
}

void mempool_insert(mem_pool_t *pool, void*elem)
{
	void *ptr = NULL;
	int elem_len = pool->ops->get_length();

	if (pool->magic != MEM_POOL_MAGIC) {
		return;
	}

	pool->operating = 1;
	int next = (pool->index + 1) % pool->length;

	ptr = pool->addr + next * elem_len;
	memcpy(ptr, elem, elem_len);

	if (next == 0) {
		pool->batch += 1;
	}

	pool->index = next;
	pool->operating = 0;
}

int mempool_peek(mem_pool_t *pool, void *elem, pool_context_t *ctx)
{
	void *ptr = NULL;
	int idx = pool->index;
	int64_t read_offset = ctx->batch * pool->length + ctx->index;
	int64_t write_offset = (int64_t)(pool->batch * pool->length) + pool->index;
	int elem_len = pool->ops->get_length();

	if (pool->magic != MEM_POOL_MAGIC) {
		return MEM_POOL_NOT_INIT;
	}

	if ((write_offset <= 0) || (read_offset >= write_offset)) {
		return MEM_POOL_NODATA;
	}

	if (!elem) {
		return MEM_POOL_PARAMS_ILLEGAL;
	}

	if (pool->operating) {
		return MEM_POOL_BUSY;
	}

	ptr = pool->addr + idx * elem_len;
	memcpy(elem, ptr, elem_len);
	ctx->batch = pool->batch;
	ctx->index = idx;

	return MEM_POOL_SUCCESS;
}

int mempool_peek_index(mem_pool_t *pool,  void *elem, int index)
{
	void *ptr = NULL;
	int idx = pool->index;
	int read_offset = 0;
	int elem_len = pool->ops->get_length();

	if (pool->magic != MEM_POOL_MAGIC) {
		return MEM_POOL_NOT_INIT;
	}

	if (index > pool->length) {
		return MEM_POOL_NODATA;
	}

	if (!elem) {
		return MEM_POOL_PARAMS_ILLEGAL;
	}

	read_offset = idx >= index ? idx - index : pool->length + idx - index;
	ptr = pool->addr + read_offset * elem_len;
	memcpy(elem, ptr, elem_len);

	return MEM_POOL_SUCCESS;
}


