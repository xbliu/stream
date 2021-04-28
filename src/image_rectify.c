#include "image_rectify.h"


static void* get_rect_frame_proc(void* arg)
{
	int32_t ret = 0;
	rect_image_t rect_image = {0};
	image_proc_task_t *task = (image_proc_task_t *)arg;
	
	while(task->running){
		rect_image.stream_id = task->stream_id;
		//to do (add calibration ref)
		ret = platform_get_rect_frame(NULL,&rect_image,-1);
		if(0 != ret) {
	    	usleep(30000);
	        continue;
	    }

		memcpy(&rect_image.dst,&rect_image.src,sizeof(image_buffer_t)); //will modify
#if 0		
	    rect_image.dst.left.pts = rect_image.src.left.pts;
	    rect_image.dst.right.pts = rect_image.src.right.pts;
	    rect_image.dst.left.trigger_time = rect_image.src.left.trigger_time;
	    rect_image.dst.right.trigger_time = rect_image.src.right.trigger_time;
		rect_image.dst.left.ref_time = rect_image.src.left.ref_time;
		rect_image.dst.right.ref_time = rect_image.src.right.ref_time;
		rect_image.dst.left.average = rect_image.src.left.average;
		rect_image.dst.right.average = rect_image.src.right.average;
#endif		
	    
	    //item_ring_buffer_push(&pService->subStreams[stream].rectFrames,&rectImage,sizeof(DPU_RECT_IMAGE_S));
		image_mempool_insert(task->pool_handle, task->type, task->stream_id, &rect_image);
	}

	return NULL;
}


int image_rectify_init()
{
	return 0;
}

int image_rectify_destroy()
{
	return 0;
}


