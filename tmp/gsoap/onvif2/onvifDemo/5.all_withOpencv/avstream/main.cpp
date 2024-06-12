/************************************************************************
**
** 作者：许振坪
** 日期：2017-05-03
** 博客：http://blog.csdn.net/benkaoya
** 描述：读取IPC音视频流数据示例代码
**
************************************************************************/
#include <iostream>

#include "onvif_comm.h"
#include "onvif_dump.h"

extern "C"{
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/pixdesc.h"
}

#include <opencv2/opencv.hpp>

/************************************************************************
**函数：open_rtsp
**功能：从RTSP获取音视频流数据
**参数：
        [in]  rtsp - RTSP地址
**返回：无
************************************************************************/
void open_rtsp(char *rtsp)
{
    unsigned int    i;
    int             ret, got_picture;
    int             video_st_index = -1;
    int             audio_st_index = -1;
    AVFormatContext *ifmt_ctx = NULL;
    AVPacket        *packet;
    AVStream        *st = NULL;
    char            errbuf[64];
    //
    AVCodecContext* pCodecCtx; //ffmpeg解码类的类成员
    AVCodec *pCodec; //解码器指针
    AVFrame *pAvFrame, *pFrameBGR; //多媒体帧，保存解码后的数据帧

    av_register_all();                                                          // Register all codecs and formats so that they can be used.
    avformat_network_init();                                                    // Initialization of network components

    if ((ret = avformat_open_input(&ifmt_ctx, rtsp, 0, NULL)) < 0) {            // Open the input file for reading.
        printf("Could not open input file '%s' (error '%s')\n", rtsp, av_make_error_string(errbuf, sizeof(errbuf), ret));
        goto EXIT;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {                // Get information on the input file (number of streams etc.).
        printf("Could not open find stream info (error '%s')\n", av_make_error_string(errbuf, sizeof(errbuf), ret));
        goto EXIT;
    }

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {                                // dump information
        av_dump_format(ifmt_ctx, i, rtsp, 0);
    }

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {                                // find video stream index
        st = ifmt_ctx->streams[i];
        switch(st->codec->codec_type) {
        case AVMEDIA_TYPE_AUDIO: audio_st_index = i; break;
        case AVMEDIA_TYPE_VIDEO: video_st_index = i; break;
        default: break;
        }
    }
    if (-1 == video_st_index) {
        printf("No H.264 video stream in the input file\n");
        goto EXIT;
    }

    pCodecCtx = ifmt_ctx->streams[video_st_index]->codec; //得到一个指向视频流的上下文指针
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id); //到该格式的解码器
    if (pCodec == NULL) {
        printf("Cant't find the decoder !\n"); //寻找解码器
        return ;
    }
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) { //打开解码器
        printf("Can't open the decoder !\n");
        return ;
    }

    pAvFrame = av_frame_alloc(); //分配帧存储空间
    pFrameBGR = av_frame_alloc(); //存储解码后转换的RGB数据

    // 保存BGR，opencv中是按BGR来保存的
    int size;
    uint8_t *out_buffer;
    size = avpicture_get_size(AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
    out_buffer = (uint8_t *)av_malloc(size);
    avpicture_fill((AVPicture *)pFrameBGR, out_buffer, AV_PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);

    packet = (AVPacket*)malloc(sizeof(AVPacket));
    printf("-----------输出文件信息---------\n");
    av_dump_format(ifmt_ctx, 0, rtsp, 0);
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

    while (1)
    {
        do {
            ret = av_read_frame(ifmt_ctx, packet);                                // read frames
        } while (ret == AVERROR(EAGAIN));

        if (ret < 0) {
            printf("Could not read frame (error '%s')\n", av_make_error_string(errbuf, sizeof(errbuf), ret));
            break;
        }

        if (packet->stream_index == video_st_index) {                               // video frame
            printf("Video Packet size = %d\n", packet->size);

            ret = avcodec_decode_video2(pCodecCtx, pAvFrame, &got_picture, packet);
            if (ret < 0){
                printf("Decode Error.（解码错误）\n");
                return ;
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
				
				cv::Mat mRGB(cv::Size(pCodecCtx->width, pCodecCtx->height), CV_8UC3);
				mRGB.data =(uchar*)pFrameBGR->data[0];//注意不能写为：(uchar*)pFrameBGR->data

//********************************************* 可以在这里写图像处理的算法 ******************************************

				cv::imshow("RGB", mRGB);
				cv::waitKey(30);
			}
		} else if(packet->stream_index == audio_st_index) {                         // audio frame
            printf("Audio Packet size = %d\n", packet->size);
        } else {
            printf("Unknow Packet size = %d\n", packet->size);
        }

        av_packet_unref(packet);
    }

EXIT:

    if (ifmt_ctx != NULL) {
        avformat_close_input(&ifmt_ctx);
        ifmt_ctx = NULL;
    }
	if(out_buffer != NULL){
		av_free(out_buffer);
		out_buffer = NULL;
	}
	if(pFrameBGR != NULL){
		av_free(pFrameBGR);
		pFrameBGR = NULL;
	}
	if(pAvFrame != NULL){
		av_free(pAvFrame);
		pAvFrame = NULL;
	}
	if(pCodecCtx != NULL){
		avcodec_close(pCodecCtx);
		pCodecCtx = NULL;
	}
 
	if(img_convert_ctx != NULL){
		sws_freeContext(img_convert_ctx);
		img_convert_ctx = NULL;
	}
    cvDestroyWindow("RGB");

    return ;
}

void cb_discovery(char *DeviceXAddr)
{
    int stmno = 1;                                                              // 码流序号，0为主码流，1为辅码流
    int profile_cnt = 0;                                                        // 设备配置文件个数
    struct tagProfile *profiles = NULL;                                         // 设备配置文件列表
    struct tagCapabilities capa;                                                // 设备能力信息

    char uri[ONVIF_ADDRESS_SIZE] = {0};                                         // 不带认证信息的URI地址
    char uri_auth[ONVIF_ADDRESS_SIZE + 50] = {0};                               // 带有认证信息的URI地址

    ONVIF_GetCapabilities(DeviceXAddr, &capa);                                  // 获取设备能力信息（获取媒体服务地址）

    profile_cnt = ONVIF_GetProfiles(capa.MediaXAddr, &profiles);                // 获取媒体配置信息（主/辅码流配置信息）

    if (profile_cnt > stmno) {
        ONVIF_GetStreamUri(capa.MediaXAddr, profiles[stmno].token, uri, sizeof(uri)); // 获取RTSP地址

        make_uri_withauth(uri, USERNAME, PASSWORD, uri_auth, sizeof(uri_auth)); // 生成带认证信息的URI（有的IPC要求认证）

		std::cout << "uri is: " << uri << std::endl;
		std::cout << "uri_auth is: " << uri_auth << std::endl;

        open_rtsp(uri_auth);                                                    // 读取主码流的音视频数据
    }

    if (NULL != profiles) {
        free(profiles);
        profiles = NULL;
    }
}

int main(int argc, char **argv)
{
    ONVIF_DetectDevice(cb_discovery);

    return 0;
}
