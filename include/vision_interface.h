#ifndef _VISION_INTERFACE_H_
#define _VISION_INTERFACE_H_

#include <stdint.h>


typedef struct {
	uint64_t addr[3];
	uint32_t stride[3];
	uint32_t width;
	uint32_t height;
	uint32_t pixel_format;
	
	void *priv;
} image_object_t;

typedef struct {
	image_object_t image;

	uint64_t pts;
	uint64_t trigger_time;
	uint32_t average;
	uint32_t ref_time; //reference time
	uint32_t video_format;
	
	void *priv;
} frame_object_t;

typedef struct {
	frame_object_t left;
	frame_object_t right;
} image_buffer_t;

typedef struct {
	int stream_id;
	image_buffer_t src;
	image_buffer_t dst;
} rect_image_t;

typedef struct {
	rect_image_t rect;
	frame_object_t depth_frame;
} depth_image_t;

typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t pixel_format;
} image_desc_t;

typedef struct {
	uint8_t running;
	uint32_t type;
	uint16_t stream_id;
	void *pool_handle;
} image_proc_task_t;


/*
stream:		which way
dpu_type: 	rect or depth
*/
void *vision_dpu_frame_open(int stream_id, int dpu_type);
int vision_get_rect_frame(void *handle, rect_image_t *rect_image, int timeout);
int vision_get_match_frame(void *handle, depth_image_t *depth_image, int timeout);
int vision_dpu_frame_close(void *handle);

int vision_alloc_frame(image_desc_t *desc, image_object_t *image);
int vision_csc_frame(image_object_t *src, image_object_t *dst, int timeout);
int vision_csc_fov_frame(image_object_t *src, image_object_t *dst, int timeout, float *hfov, float *vfov);
int vision_destroy_frame(image_desc_t *desc, image_object_t *image);


#if 0
int vision_report_soa_info(const alink_movidus_soa_report_t *soa_info); //是否需要协议无关法
int vision_get_orbit_time_lapse_shoot();
int vision_get_time_lapse_cruise_status(void *handle, int *cruise_status);
int vision_report_path_planning_info(vision_pathplanning_info info);
#endif


#endif

