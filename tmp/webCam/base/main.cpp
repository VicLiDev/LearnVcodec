//原文：https://blog.csdn.net/guyuealian/article/details/79607568 
//命令行编译指令:g++ -o webCam -w main.cpp webcamdata.cpp -lpthread $(pkg-config --libs --cflags opencv libavcodec libavdevice libavfilter libavformat libavutil)
// 该程序是使用网络摄像头的基本程序，可以在下边的main函数中设计自己的图像处理算法

#include <stdio.h>
#include <iostream>
// Opencv
#include <opencv2/opencv.hpp>
 
extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <assert.h>

#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
//新版里的图像转换结构需要引入的头文件
#include "libswscale/swscale.h"
};

#include "thread_pool.h"
#include "webcamdata.h"

void* pullWebImg(void *arg)
{
    WebCamData *camData = (WebCamData *)arg;
	AVCodec *pCodec; //解码器指针
	AVCodecContext* pCodecCtx; //ffmpeg解码类的类成员
	AVFrame* pAvFrame; //多媒体帧，保存解码后的数据帧
	AVFormatContext* pFormatCtx; //保存视频流的信息
 
	av_register_all(); //注册库中所有可用的文件格式和编码器
	//Network
	avformat_network_init();
 
	pFormatCtx = avformat_alloc_context();
	if (avformat_open_input(&pFormatCtx, camData->webCamID.c_str(), NULL, NULL) != 0) { //检查文件头部
		printf("Can't find the stream!\n");
	}
	if (avformat_find_stream_info(pFormatCtx, NULL)<0) { //查找流信息
		printf("Can't find the stream information !\n");
	}
 
	int videoindex = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; ++i){ //遍历各个流，找到第一个视频流,并记录该流的编码信息
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			break;
		}
	}
	if (videoindex == -1) {
		printf("Don't find a video stream !\n");
		return NULL;
	}
	pCodecCtx = pFormatCtx->streams[videoindex]->codec; //得到一个指向视频流的上下文指针
	pCodec = avcodec_find_decoder(pCodecCtx->codec_id); //到该格式的解码器
	if (pCodec == NULL) {
		printf("Cant't find the decoder !\n"); //寻找解码器
		return NULL;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) { //打开解码器
		printf("Can't open the decoder !\n");
		return NULL;
	}
 
	pAvFrame = av_frame_alloc(); //分配帧存储空间
	AVFrame* pFrameBGR = av_frame_alloc(); //存储解码后转换的RGB数据
 
 
 
	// 保存BGR，opencv中是按BGR来保存的
	int size = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
	uint8_t *out_buffer = (uint8_t *)av_malloc(size);
	avpicture_fill((AVPicture *)pFrameBGR, out_buffer, AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
 
	AVPacket* packet = (AVPacket*)malloc(sizeof(AVPacket));
	printf("-----------输出文件信息---------\n");
	av_dump_format(pFormatCtx, 0, camData->webCamID.c_str(), 0);
	printf("------------------------------");
 
	struct SwsContext *img_convert_ctx;
	img_convert_ctx = sws_getContext(pCodecCtx->width,
		pCodecCtx->height, 
		pCodecCtx->pix_fmt, 
		pCodecCtx->width, 
		pCodecCtx->height,
		AV_PIX_FMT_BGR24, //设置sws_scale转换格式为BGR24，这样转换后可以直接用OpenCV显示图像了
		SWS_BICUBIC, 
		NULL, NULL, NULL);
 
	int ret;
	int got_picture;
 
	// cvNamedWindow("RGB", 1);
	for (;;){
		if (av_read_frame(pFormatCtx, packet) >= 0){
			if (packet->stream_index == videoindex){
				ret = avcodec_decode_video2(pCodecCtx, pAvFrame, &got_picture, packet);
				if (ret < 0){
					printf("Decode Error.（解码错误）\n");
					return NULL;
				}
				if (got_picture){
					//YUV to RGB
					sws_scale(img_convert_ctx, 
						(const uint8_t* const*)pAvFrame->data, 
						pAvFrame->linesize, 
						0, 
						pCodecCtx->height,
						pFrameBGR->data, 
						pFrameBGR->linesize);
					//Mat mRGB(pCodecCtx->height, pCodecCtx->width, CV_8UC3, out_buffer);//（1）等效于下面
					//Mat mRGB(Size(pCodecCtx->width,pCodecCtx->height), CV_8UC3);//（2）
					//mRGB.data = out_buffer;//memcpy(pCvMat.data, out_buffer, size);
					
					cv::Mat mRGB(cv::Size(pCodecCtx->width, pCodecCtx->height), CV_8UC3);
					mRGB.data =(uchar*)pFrameBGR->data[0];//注意不能写为：(uchar*)pFrameBGR->data
                    camData->loadImg(mRGB);
					// imshow("RGB", mRGB);
                    // char key = (char)cv::waitKey(1);
                    // if( key == 27 )
                    //     break;
				}
			}
			av_free_packet(packet);
		}else{
			break;
		}
	}
 
	av_free(out_buffer);
	av_free(pFrameBGR);
	av_free(pAvFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
 
	sws_freeContext(img_convert_ctx);
	// cvDestroyWindow("RGB");
 
	system("pause");
	return NULL;
}

int main(int argc, char **argv)
{
	//========================== pullImg
	// 需要添加更多摄像头的时候只需要在这里加入链接，然后把他创建的 WebCamData 对象压到栈里就可以了，然后就可以在后边写算法了
    std::vector<WebCamData> m_webCamDataV;
    std::string camID1 = "rtsp://192.168.1.211:554/user=admin_password=tlJwpbo6_channel=1_stream=1.sdp?real_stream";
    std::string camID2 = "rtsp://192.168.1.167:554/user=admin_password=tlJwpbo6_channel=1_stream=1.sdp?real_stream";
    m_webCamDataV.push_back(WebCamData(camID1));
    m_webCamDataV.push_back(WebCamData(camID2));

    pool_init (3);/*线程池中最多三个活动线程*/
    for(int i = 0; i < m_webCamDataV.size(); i++){
        pool_add_worker(pullWebImg, &m_webCamDataV[i]);
    }
    //========================== pullImg end

    std::vector<cv::Mat> views(m_webCamDataV.size());
    for(int i = 0;;i++){
    	for(int i = 0; i < m_webCamDataV.size(); i++){
    		m_webCamDataV[i].getImg(views[i]);
    	}

    	for(int i = 0; i < views.size(); i++){
    		if(views[i].empty()){
    			std::cout << "img " << std::to_string(i) << " is empty!!" << std::endl;
    			continue;
    		}else{
    			// 工作时间长了以后画面显示会卡住，但是通过 imwrite 可以看到，获取图像的线程是在正常工作的
    			cv::imshow("getImg" + std::to_string(i), views[i]);
    			cv::imwrite("getImg" + std::to_string(i) + ".jpg", views[i]);
    		}
    	}
    	// 延时不能太短，不然加载图像的线程无法获取互斥锁，如果这里有耗时比较久的算法可以减少延时时长
    	char key = (char)cv::waitKey(1);
    	if( key == 27 )
    		break;

    	//===================================== 算法
    	// ...
    	
    	//===================================== 算法 end
    }

    //释放线程
    pool_destroy ();

}
