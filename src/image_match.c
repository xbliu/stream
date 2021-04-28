
typedef struct {
	uint8_t running;
	
} image_match_proc_t;

static void* get_match_frame_proc(void* arg)
{
	#define	 FRAME_RETRY_TIMES				(20)
	HI_U16 stream;
	vision_service_t *pService = get_camera_service();
	stream = *((HI_U16*)arg);
	DPU_MATCH_IMAGE_S matchImage;
	DPU_RECT_IMAGE_S rectImage;
	DPU_CUSTOM_RECT_IMAGE_S customRectImage;
	pool_context_t customRectCtx = {0};
	pool_context_t rectCtx = {0};
	HI_S32 s32Ret = HI_SUCCESS;
	HI_U32 readValid = 0, rightFrame = 0;
	int tryTimes = 0, searchRange = 1, searchIndex = 0;
	
	int PhotoSaveId = 0, framerate;
	HI_U8 * VirAddr;
	HI_U8 * LeftVirAddr;
	HI_U8 * RightVirAddr;
	HI_U64 sendflag = 0, deltaTime = 0;
	pthread_t tmptid;
	uint32_t directions = 0;
	VISION_CALIBRATION_STATUS* pCalibStatus = NULL;
	
	depth_image_t depth_image;	

	while(pService->running){
		depth_image.rect.stream_id = stream;
		//to do (add calibration ref)

		s32Ret = HI_MPI_DPU_MATCH_GetFrame(matchImage.dpuGrp, &matchImage.srcLeft, &matchImage.srcRight, &matchImage.dstFrame, -1);
	    if(s32Ret != HI_SUCCESS)
	    {     
	    	usleep(30000);
	        continue;
	    }

		tryTimes = 0;
		searchIndex = 0;
		//Match通道非绑定，从buffer中取最新的rect图像，以rect图像的时间戳为深度图赋值
		if((DPU_CUSTOM_GROUPS & (1 << stream))){				
			//重复若干次获取自处理的rect图像来匹配match，应保证重复次数足够多以避免无法获取到校正图像，导致深度图时间戳不正确
			while(tryTimes ++ < FRAME_RETRY_TIMES && HI_SUCCESS != (s32Ret = imageMemCustomRectImage_peek(pService->imagePool, stream, &customRectImage, &customRectCtx))){
				usleep(1000);
			}
			
			if(HI_SUCCESS == s32Ret){
				memcpy(&matchImage.srcLeft, &customRectImage.leftSrcImage, sizeof(VIDEO_FRAME_INFO_S));
				memcpy(&matchImage.srcRight, &customRectImage.rightSrcImage, sizeof(VIDEO_FRAME_INFO_S));
				memcpy(&matchImage.rectLeft, &customRectImage.leftImage, sizeof(VIDEO_FRAME_INFO_S));
				memcpy(&matchImage.rectRight, &customRectImage.rightImage, sizeof(VIDEO_FRAME_INFO_S));
			}
		}else{
			//重复若干次获取rect图像来匹配match，应保证重复次数足够多以避免无法获取到校正图像，导致深度图时间戳不正确
			while(tryTimes ++ < FRAME_RETRY_TIMES && HI_SUCCESS != (s32Ret = imageMemRectImage_peek(pService->imagePool, stream, &rectImage, &rectCtx))){
				usleep(1000);
			}
			if(HI_SUCCESS == s32Ret){
				rectImage.dstLeft.stVFrame.u64PTS = rectImage.srcLeft.stVFrame.u64PTS;
	    		rectImage.dstRight.stVFrame.u64PTS = rectImage.srcRight.stVFrame.u64PTS;
	    		rectImage.dstLeft.stVFrame.u64PrivateData = rectImage.srcLeft.stVFrame.u64PrivateData;
	    		rectImage.dstRight.stVFrame.u64PrivateData = rectImage.srcRight.stVFrame.u64PrivateData;
				rectImage.dstLeft.stVFrame.u32FrameFlag = rectImage.srcLeft.stVFrame.u32FrameFlag;
	    		rectImage.dstRight.stVFrame.u32FrameFlag = rectImage.srcRight.stVFrame.u32FrameFlag;
				rectImage.dstLeft.stVFrame.u32TimeRef = rectImage.srcLeft.stVFrame.u32TimeRef;
	    		rectImage.dstRight.stVFrame.u32TimeRef = rectImage.srcRight.stVFrame.u32TimeRef;
				memcpy(&matchImage.rectLeft, &rectImage.dstLeft, sizeof(VIDEO_FRAME_INFO_S));
				memcpy(&matchImage.rectRight, &rectImage.dstRight, sizeof(VIDEO_FRAME_INFO_S));
			}
		}

	    matchImage.dstFrame.stVFrame.u64PTS = matchImage.srcLeft.stVFrame.u64PTS;
	    matchImage.dstFrame.stVFrame.u64PrivateData = matchImage.srcLeft.stVFrame.u64PrivateData;
		matchImage.dstFrame.stVFrame.u32FrameFlag = matchImage.srcLeft.stVFrame.u32FrameFlag;
		matchImage.dstFrame.stVFrame.u32TimeRef = matchImage.srcLeft.stVFrame.u32TimeRef;
		//最新校正图像与深度图不对应，尝试往前搜索保存的校正图像以寻找匹配，搜索范围由searchRange指定
		if(matchImage.dstFrame.stVFrame.u64PrivateData < matchImage.rectLeft.stVFrame.u64PrivateData){
			while(searchIndex < searchRange){
				if((DPU_CUSTOM_GROUPS & (1 << stream))){
					s32Ret = imageMemCustomRectImage_peek_index(pService->imagePool, stream, &customRectImage, ++searchIndex);
					if(HI_SUCCESS == s32Ret && matchImage.dstFrame.stVFrame.u64PrivateData == customRectImage.leftImage.stVFrame.u64PrivateData){
						memcpy(&matchImage.srcLeft, &customRectImage.leftSrcImage, sizeof(VIDEO_FRAME_INFO_S));
						memcpy(&matchImage.srcRight, &customRectImage.rightSrcImage, sizeof(VIDEO_FRAME_INFO_S));
						memcpy(&matchImage.rectLeft, &customRectImage.leftImage, sizeof(VIDEO_FRAME_INFO_S));
						memcpy(&matchImage.rectRight, &customRectImage.rightImage, sizeof(VIDEO_FRAME_INFO_S));
					}
				}else{
					s32Ret = imageMemRectImage_peek_index(pService->imagePool, stream, &rectImage, ++searchIndex);						
					if(HI_SUCCESS == s32Ret && matchImage.dstFrame.stVFrame.u64PrivateData == rectImage.srcLeft.stVFrame.u64PrivateData){
						rectImage.dstLeft.stVFrame.u64PTS = rectImage.srcLeft.stVFrame.u64PTS;
			    		rectImage.dstRight.stVFrame.u64PTS = rectImage.srcRight.stVFrame.u64PTS;
			    		rectImage.dstLeft.stVFrame.u64PrivateData = rectImage.srcLeft.stVFrame.u64PrivateData;
			    		rectImage.dstRight.stVFrame.u64PrivateData = rectImage.srcRight.stVFrame.u64PrivateData;
						rectImage.dstLeft.stVFrame.u32FrameFlag = rectImage.srcLeft.stVFrame.u32FrameFlag;
			    		rectImage.dstRight.stVFrame.u32FrameFlag = rectImage.srcRight.stVFrame.u32FrameFlag;
						rectImage.dstLeft.stVFrame.u32TimeRef = rectImage.srcLeft.stVFrame.u32TimeRef;
			    		rectImage.dstRight.stVFrame.u32TimeRef = rectImage.srcRight.stVFrame.u32TimeRef;
						memcpy(&matchImage.rectLeft, &rectImage.dstLeft, sizeof(VIDEO_FRAME_INFO_S));
						memcpy(&matchImage.rectRight, &rectImage.dstRight, sizeof(VIDEO_FRAME_INFO_S));
					}
				}
			}
		}

		//根据深度图搜索不到时间戳匹配的校正图像，则放弃此帧
		if(matchImage.dstFrame.stVFrame.u64PrivateData != matchImage.rectLeft.stVFrame.u64PrivateData){
			dpu_match_image_release(&matchImage);
			continue;
		}
		
	    item_ring_buffer_push(&pService->subStreams[stream].matchFrames, &matchImage, sizeof(DPU_MATCH_IMAGE_S));
	    imageMemMatchImage_insert(pService->imagePool, stream, &matchImage);

		pthread_mutex_lock(&pService->mutex);
		directions = 0;
		for(uint16_t i = VISION_STREAM_FRONT; i < VISION_STREAM_BUTT; i++){
			if(pService->pCalibInfo->direction & (1 << i)){
				directions += 1;
			}
		}
		framerate = pService->pCalibInfo->framerate[stream] > 0 ? pService->pCalibInfo->framerate[stream] : 1;
		if(directions > 1){
			rightFrame = 0;
			if(pService->targetTimestamp == 0){
				pService->targetTimestamp = matchImage.srcLeft.stVFrame.u64PrivateData + 1000000000 / framerate;
			}else{
				deltaTime = pService->targetTimestamp >= matchImage.srcLeft.stVFrame.u64PrivateData ? pService->targetTimestamp - matchImage.srcLeft.stVFrame.u64PrivateData : matchImage.srcLeft.stVFrame.u64PrivateData - pService->targetTimestamp;
				if(deltaTime <= 25000000){
					pService->targetTimestamp = matchImage.srcLeft.stVFrame.u64PrivateData;
					pService->readyNumber |= (1 << stream);
					rightFrame = 1;
					if(pService->readyNumber == pService->pCalibInfo->direction){
						pService->readyNumber = 0;
						pService->targetTimestamp += 1000000000 / framerate;
					}
				}else if(deltaTime > 1000000000){
					pService->targetTimestamp = 0;
					pService->readyNumber = 0;
				}
			}
			if(!rightFrame){
				pthread_mutex_unlock(&pService->mutex);
				continue;
			}				
		}
		pthread_mutex_unlock(&pService->mutex);

        if((directions > 1 && (pService->pCalibInfo->direction & (1 << stream)) && rightFrame) || 
			(directions == 1 && (pService->pCalibInfo->direction & (1 << stream)) && sendflag % (20 / framerate) == 0))
        {
			thread_bind_cpu(2);
			sem_wait(&pService->uploadSem[stream]);
			//VISION_CONTROL_LOG(VISION_LOG_LEVEL_INFO, "%llu, %llu\n", matchImage.srcLeft.stVFrame.u64PrivateData, matchImage.srcLeft.stVFrame.u64PTS);
			if(pService->pCalibInfo->uploadObjs[stream] & AUTEL_CALIB_FRAME_SRC)
			{
				VisionThreadCreate(&tmptid, 78, -1, "VI_UPLOAD_PROC", upload_vi_frame_proc, (void *)&matchImage);
			}else{
				sem_post(&pService->uploadSem[stream]);
			}

			if(pService->pCalibInfo->uploadObjs[stream] & AUTEL_CALIB_FRAME_RECT)
			{
                LeftVirAddr = (HI_U8 * )HI_MPI_SYS_MmapCache(matchImage.rectLeft.stVFrame.u64PhyAddr[0], matchImage.rectLeft.stVFrame.u32Stride[0] * matchImage.rectLeft.stVFrame.u32Height);
                RightVirAddr = (HI_U8 * )HI_MPI_SYS_MmapCache(matchImage.rectRight.stVFrame.u64PhyAddr[0], matchImage.rectRight.stVFrame.u32Stride[0] * matchImage.rectRight.stVFrame.u32Height);

				if(pService->pCalibInfo->imageType[stream] == AUTEL_CALIB_FRAME_TYPE_RAW)
				{
					vision_tcp_buf_insert_part(AUTEL_CALIB_GET_FRAMES,
					 	pService->pCalibInfo->imageType[stream],
					 	AUTEL_CALIB_FRAME_RECT,
					 	1 << stream,
					 	matchImage.dstFrame.stVFrame.u64PrivateData, 
					 	matchImage.dstFrame.stVFrame.u64PTS,
					 	LeftVirAddr,
					 	RightVirAddr,
					 	matchImage.rectLeft.stVFrame.u32Width,
					 	matchImage.rectLeft.stVFrame.u32Height,
					 	matchImage.rectLeft.stVFrame.u32Stride[0]);
				}
				else if(pService->pCalibInfo->imageType[stream] == AUTEL_CALIB_FRAME_TYPE_PNG)
				{
					DualImageEncodeSend(LeftVirAddr,
					 	RightVirAddr, 
					 	matchImage.rectLeft.stVFrame.u32Stride[0],
					 	matchImage.rectLeft.stVFrame.u32Height,  
					 	AUTEL_CALIB_FRAME_RECT,
					 	1 << stream,
					 	matchImage.dstFrame.stVFrame.u64PrivateData, 
					 	matchImage.rectLeft.stVFrame.u64PTS);
				}
						 
				HI_MPI_SYS_Munmap(LeftVirAddr, matchImage.rectLeft.stVFrame.u32Stride[0] * matchImage.rectLeft.stVFrame.u32Height);
				HI_MPI_SYS_Munmap(RightVirAddr, matchImage.rectRight.stVFrame.u32Stride[0] * matchImage.rectRight.stVFrame.u32Height);
			}

			if(pService->pCalibInfo->uploadObjs[stream] & AUTEL_CALIB_FRAME_MATCH)
			{
		        VirAddr = (HI_U8 * )HI_MPI_SYS_MmapCache(matchImage.dstFrame.stVFrame.u64PhyAddr[0], matchImage.dstFrame.stVFrame.u32Stride[0] * matchImage.dstFrame.stVFrame.u32Height);

		        if(pService->pCalibInfo->imageType[stream] == AUTEL_CALIB_FRAME_TYPE_RAW)
		        {
		            vision_tcp_buf_insert(AUTEL_CALIB_GET_FRAMES,
				 	pService->pCalibInfo->imageType[stream],
				 	AUTEL_CALIB_FRAME_MATCH,
				 	1 << stream,
				 	matchImage.dstFrame.stVFrame.u64PrivateData,
				 	matchImage.dstFrame.stVFrame.u64PTS,
				 	VirAddr,
				 	matchImage.dstFrame.stVFrame.u32Stride[0] * matchImage.dstFrame.stVFrame.u32Height);
		        }
		        else if(pService->pCalibInfo->imageType[stream] == AUTEL_CALIB_FRAME_TYPE_PNG)
		        {
		            SingleImageEncodeSend(VirAddr,
						matchImage.dstFrame.stVFrame.u32Stride[0],
						matchImage.dstFrame.stVFrame.u32Height,
						AUTEL_CALIB_FRAME_MATCH, 
				 		1 << stream,
					 	matchImage.dstFrame.stVFrame.u64PrivateData, 
						matchImage.dstFrame.stVFrame.u64PTS);
		        }
					 
				HI_MPI_SYS_Munmap(VirAddr, matchImage.dstFrame.stVFrame.u32Stride[0] * matchImage.dstFrame.stVFrame.u32Height);
			}

			if(pService->pCalibInfo->framerate[stream] == 0){
				pService->pCalibInfo->direction &= (~(1 << stream));
			}
			PhotoSaveId++;
		}

		sendflag ++;			
	}
	sem_destroy(&pService->uploadSem[stream]);
	return NULL;
}


