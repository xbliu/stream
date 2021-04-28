#include "image_match.h"


#define FRAME_RETRY_TIMES (20)


static void* get_match_frame_proc(void* arg)
{
	int ret = 0;
	int tryTimes = 0;
	int search_index = 0;
	int search_range = 1;
	pool_context_t context = {0};
	rect_image_t rect_image = {0};
	depth_image_t depth_image = {0};
	
	image_proc_task_t task = (image_proc_task_t *)arg;
	while(task->running) {
		depth_image.rect.stream_id = task->stream_id;
		//to do (add calibration ref)

		ret = platform_get_match_frame(NULL, &depth_image, -1);
	    if(0 != ret) {     
	    	usleep(30000);
	        continue;
	    }

		tryTimes = 0;
		search_index = 0;
		//重复若干次获取自处理的rect图像来匹配match，应保证重复次数足够多以避免无法获取到校正图像，导致深度图时间戳不正确
		do {
			ret = image_mempool_peek(task->pool_handle, task->stream_id, &rect_image, &context);
		} while (tryTimes ++ < FRAME_RETRY_TIMES && (0 != ret);

		if (0 == ret) {
			memcpy(rect_image.dst,rect_image.src,sizeof(image_buffer_t));
			memcpy(&depth_image.rect,&rect_image,sizeof(rect_image_t));
		}

		memcpy(&depth_image.depth_frame,&depth_image.rect.src.left,sizeof(frame_object_t));
		
		//最新校正图像与深度图不对应，尝试往前搜索保存的校正图像以寻找匹配，搜索范围由searchRange指定
		if(depth_image.depth_frame.trigger_time < depth_image.rect.src.left.trigger_time){
			while(search_index < search_range){
				ret = image_mempool_peek_index(task->pool_handle, task->stream_id, &rect_image, ++search_index);
				if(0 == ret && depth_image.depth_frame.trigger_time == rect_image.dst.left.trigger_time) {
					memcpy(rect_image.dst,rect_image.src,sizeof(image_buffer_t));
					memcpy(&depth_image.rect,&rect_image,sizeof(rect_image_t));
				}
			}
		}

		//根据深度图搜索不到时间戳匹配的校正图像，则放弃此帧
		if(depth_image.depth_frame.trigger_time != depth_image.rect.dst.left.trigger_time){
			dpu_match_image_release(&matchImage);
			continue;
		}
		
	    //item_ring_buffer_push(&pService->subStreams[stream].matchFrames, &matchImage, sizeof(DPU_MATCH_IMAGE_S));
		image_mempool_insert(task->pool_handle, task->type, task->stream_id, &depth_image);
	}
	
	return NULL;
}

int image_match_init()
{
	return 0;
}

int image_match_destroy()
{
	return 0;
}


