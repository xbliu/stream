#include "image_rectify.h"


typedef struct {
	uint8_t running;
	uint32_t type;
	uint16_t stream_id;
	void *pool_handle;
} image_rectify_t;


static void* get_rect_frame_proc(void* arg)
{
	int32_t ret = 0;
	rect_image_t rect_image = {0};
	image_rectify_t *rectify = (image_rectify_t *)arg;
	
	while(rectify->running){
		rect_image.stream_id = rectify->stream_id;
		//to do (add calibration ref)
		ret = platform_get_rect_frame(NULL,&rect_image,-1);
		if(0 != ret) {
	    	usleep(30000);
	        continue;
	    }
	    
	    rect_image.dst.left.pts = rect_image.src.left.pts;
	    rect_image.dst.right.pts = rect_image.src.right.pts;
	    rect_image.dst.left.trigger_time = rect_image.src.left.trigger_time;
	    rect_image.dst.right.trigger_time = rect_image.src.right.trigger_time;
		rect_image.dst.left.ref_time = rect_image.src.left.ref_time;
		rect_image.dst.right.ref_time = rect_image.src.right.ref_time;
		rect_image.dst.left.average = rect_image.src.left.average;
		rect_image.dst.right.average = rect_image.src.right.average;
	    
	    //item_ring_buffer_push(&pService->subStreams[stream].rectFrames,&rectImage,sizeof(DPU_RECT_IMAGE_S));
		image_mempool_insert(rectify->pool_handle, rectify->type, rectify->stream_id, &rect_image);
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


