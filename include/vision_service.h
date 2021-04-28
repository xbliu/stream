#ifndef _VISION_SERVICE_H_
#define _VISION_SERVICE_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* Begin of #ifdef __cplusplus */


typedef enum {
	VISION_STREAM_FRONT = 0,
	VISION_STREAM_REAR,
	VISION_STREAM_BOTTOM,
	VISION_STREAM_RIGHT,
	VISION_STREAM_LEFT,
	VISION_STREAM_TOP,	  
	VISION_STREAM_MAIN,
	VISION_STREAM_BUTT = VISION_STREAM_MAIN
} vision_stream_e;

typedef struct{
	uint32_t direction; 						//当前标定方向
	uint32_t imu;								//需要上传的传感器数据集合
	uint32_t image_type[VISION_STREAM_BUTT];
	uint32_t frame_rate[VISION_STREAM_BUTT];
	uint32_t upload_objs[VISION_STREAM_BUTT];
} calib_info_t;


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* Begin of #ifdef __cplusplus */

#endif

