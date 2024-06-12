//原文：https://blog.csdn.net/guyuealian/article/details/79607568 
//命令行编译指令:g++ -o webCali -w web_cali.cpp -lpthread $(pkg-config --libs --cflags opencv libavcodec libavdevice libavfilter libavformat libavutil)
// 该程序用于校正网络摄像头，当使用不同的网络设想头的时候需要更改摄像头地址，即后边的 filename 变量


#define __STDC_CONSTANT_MACROS
#include <stdio.h>
#include <iostream>
// Opencv
#include <opencv/cv.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
 
 
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

//============================================================================================= begin1
#include "opencv2/core.hpp"
#include <opencv2/core/utility.hpp>
#include "opencv2/imgproc.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"

#include <cctype>
#include <stdio.h>
#include <string.h>
#include <time.h>

using namespace cv;
using namespace std;

//============================================================================================= end1

static pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER;
cv::Mat webImg; //该数据不要直接访问，通过loadImg和getImg访问

int loadImg(cv::Mat &img)
{
    // webImg.create(img.size(), CV_8UC3);
    pthread_mutex_lock(&m_mutex);
    img.copyTo(webImg);
    pthread_mutex_unlock(&m_mutex);
    // imshow("after load", webImg);
    // waitKey(1);
    // std::cout << "raw data size: " << img.size() << std::endl;
    // std::cout << "webImg data size: " << webImg.size() << std::endl;
}

// 在调用该函数之后要注意判断数据是否为空
// 例如： getImg(view);
//       if(view.empty())
//            continue;
int getImg(cv::Mat &getImg)
{
    pthread_mutex_lock(&m_mutex);
    webImg.copyTo(getImg);
    pthread_mutex_unlock(&m_mutex);
    // imshow("before get", webImg);
    // waitKey(1);
}

/*
*线程池里所有运行和等待的任务都是一个CThread_worker
*由于所有任务都在链表里，所以是一个链表结构
*/

typedef struct worker{
    /*回调函数，任务运行时会调用此函数，注意也可声明成其它形式*/
    void *(*process) (void *arg);
    void *arg;/*回调函数的参数*/
    struct worker *next;
} CThread_worker;

/*线程池结构*/
typedef struct{
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;
    /*链表结构，线程池中所有等待任务*/
    CThread_worker *queue_head;
    /*保存所有线程ID*/
    pthread_t *threadid;
    /*线程池中允许的活动线程数目*/
    int max_thread_num;
    /*当前等待队列的任务数目*/
    int cur_queue_size;
    /*是否销毁线程池*/
    int shutdown;
} CThread_pool;

int pool_add_worker (void *(*process) (void *arg), void *arg);
void *thread_routine (void *arg);

static CThread_pool *pool = NULL;

void pool_init(int max_thread_num)
{
    pool = (CThread_pool *) malloc (sizeof (CThread_pool));
    pthread_mutex_init (&(pool->queue_lock), NULL);
    pthread_cond_init (&(pool->queue_ready), NULL);
    pool->queue_head = NULL;
    pool->max_thread_num = max_thread_num;
    pool->cur_queue_size = 0;
    pool->shutdown = 0;
    pool->threadid = (pthread_t *) malloc (max_thread_num * sizeof (pthread_t));
    for(int i = 0; i < max_thread_num; i++){ 
        pthread_create (&(pool->threadid[i]), NULL, thread_routine, NULL);
    }
}

/*向线程池中加入任务*/
int pool_add_worker(void *(*process) (void *arg), void *arg)
{
    /*构造一个新任务*/
    CThread_worker *newworker = (CThread_worker *) malloc (sizeof (CThread_worker));
    newworker->process = process;
    newworker->arg = arg;
    newworker->next = NULL;/*别忘置空*/
    pthread_mutex_lock (&(pool->queue_lock));
    /*将任务加入到等待队列中*/
    CThread_worker *member = pool->queue_head;
    if (member != NULL){
        while (member->next != NULL)
            member = member->next;
        member->next = newworker;
    }else{
        pool->queue_head = newworker;
    }
    assert (pool->queue_head != NULL);
    pool->cur_queue_size++;
    pthread_mutex_unlock (&(pool->queue_lock));
    /*好了，等待队列中有任务了，唤醒一个等待线程；
    注意如果所有线程都在忙碌，这句没有任何作用*/
    pthread_cond_signal (&(pool->queue_ready));
    return 0;
}

/*销毁线程池，等待队列中的任务不会再被执行，但是正在运行的线程会一直把任务运行完后再退出*/
int pool_destroy ()
{
    if (pool->shutdown)
        return -1;/*防止两次调用*/
    pool->shutdown = 1;
    /*唤醒所有等待线程，线程池要销毁了*/
    pthread_cond_broadcast (&(pool->queue_ready));
    /*阻塞等待线程退出，否则就成僵尸了*/
    for(int i = 0; i < pool->max_thread_num; i++)
        pthread_join (pool->threadid[i], NULL);
    free (pool->threadid);
    /*销毁等待队列*/
    CThread_worker *head = NULL;
    while (pool->queue_head != NULL){
        head = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        free (head);
    }
    /*条件变量和互斥量也别忘了销毁*/
    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_ready));
    
    free (pool);
    /*销毁后指针置空是个好习惯*/
    pool=NULL;
    return 0;
}

void * thread_routine (void *arg)
{
    printf ("starting thread 0x%x\n", pthread_self ());
    while (1){
        pthread_mutex_lock (&(pool->queue_lock));
        /*如果等待队列为0并且不销毁线程池，则处于阻塞状态; 注意
        pthread_cond_wait是一个原子操作，等待前会解锁，唤醒后会加锁*/
        while (pool->cur_queue_size == 0 && !pool->shutdown){
            printf ("thread 0x%x is waiting\n", pthread_self ());
            pthread_cond_wait (&(pool->queue_ready), &(pool->queue_lock));
        }
        /*线程池要销毁了*/
        if (pool->shutdown){
            /*遇到break,continue,return等跳转语句，千万不要忘记先解锁*/
            pthread_mutex_unlock (&(pool->queue_lock));
            printf ("thread 0x%x will exit\n", pthread_self ());
            pthread_exit (NULL);
        }
        printf ("thread 0x%x is starting to work\n", pthread_self ());
        /*assert是调试的好帮手*/
        assert (pool->cur_queue_size != 0);
        assert (pool->queue_head != NULL);
        
        /*等待队列长度减去1，并取出链表中的头元素*/
        pool->cur_queue_size--;
        CThread_worker *worker = pool->queue_head;
        pool->queue_head = worker->next;
        pthread_mutex_unlock (&(pool->queue_lock));
        /*调用回调函数，执行任务*/
        (*(worker->process)) (worker->arg);
        free (worker);
        worker = NULL;
    }
    /*这一句应该是不可达的*/
    pthread_exit (NULL);
}
 
void* pullWebImg(void *arg)
{
	char *camID = (char *)arg;
	AVCodec *pCodec; //解码器指针
	AVCodecContext* pCodecCtx; //ffmpeg解码类的类成员
	AVFrame* pAvFrame; //多媒体帧，保存解码后的数据帧
	AVFormatContext* pFormatCtx; //保存视频流的信息
 
	av_register_all(); //注册库中所有可用的文件格式和编码器
	//Network
	avformat_network_init();
 
	pFormatCtx = avformat_alloc_context();
	if (avformat_open_input(&pFormatCtx, camID, NULL, NULL) != 0) { //检查文件头部
		printf("Can't find the stream!\n");
	}
	if (avformat_find_stream_info(pFormatCtx, NULL)<0) { //查找流信息
		printf("Can't find the stream information !\n");
	}
 
	int videoindex = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; ++i) //遍历各个流，找到第一个视频流,并记录该流的编码信息
	{
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
	av_dump_format(pFormatCtx, 0, camID, 0);
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
	for (;;)
	{
		if (av_read_frame(pFormatCtx, packet) >= 0)
		{
			if (packet->stream_index == videoindex)
			{
				ret = avcodec_decode_video2(pCodecCtx, pAvFrame, &got_picture, packet);
				if (ret < 0)
				{
					printf("Decode Error.（解码错误）\n");
					return NULL;
				}
				if (got_picture)
				{
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
					
					Mat mRGB(Size(pCodecCtx->width, pCodecCtx->height), CV_8UC3);
					mRGB.data =(uchar*)pFrameBGR->data[0];//注意不能写为：(uchar*)pFrameBGR->data
                    loadImg(mRGB);
					// imshow("RGB", mRGB);
                    char key = (char)waitKey(1);
                    if( key == 27 )
                        break;

					//-- algorithm

				}
			}
			av_free_packet(packet);
		}
		else
		{
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

//============================================================================================= algo

enum { DETECTION = 0, CAPTURING = 1, CALIBRATED = 2 };
enum Pattern { CHESSBOARD, CIRCLES_GRID, ASYMMETRIC_CIRCLES_GRID };

static double computeReprojectionErrors(
        const vector<vector<Point3f> >& objectPoints,
        const vector<vector<Point2f> >& imagePoints,
        const vector<Mat>& rvecs, const vector<Mat>& tvecs,
        const Mat& cameraMatrix, const Mat& distCoeffs,
        vector<float>& perViewErrors )
{
    vector<Point2f> imagePoints2;
    int i, totalPoints = 0;
    double totalErr = 0, err;
    perViewErrors.resize(objectPoints.size());

    for( i = 0; i < (int)objectPoints.size(); i++ )
    {
        projectPoints(Mat(objectPoints[i]), rvecs[i], tvecs[i],
                      cameraMatrix, distCoeffs, imagePoints2);
        err = norm(Mat(imagePoints[i]), Mat(imagePoints2), NORM_L2);
        int n = (int)objectPoints[i].size();
        perViewErrors[i] = (float)std::sqrt(err*err/n);
        totalErr += err*err;
        totalPoints += n;
    }

    return std::sqrt(totalErr/totalPoints);
}

static void calcChessboardCorners(Size boardSize, float squareSize, vector<Point3f>& corners, Pattern patternType = CHESSBOARD)
{
    corners.resize(0);

    switch(patternType)
    {
      case CHESSBOARD:
      case CIRCLES_GRID:
        for( int i = 0; i < boardSize.height; i++ )
            for( int j = 0; j < boardSize.width; j++ )
                corners.push_back(Point3f(float(j*squareSize),
                                          float(i*squareSize), 0));
        break;

      case ASYMMETRIC_CIRCLES_GRID:
        for( int i = 0; i < boardSize.height; i++ )
            for( int j = 0; j < boardSize.width; j++ )
                corners.push_back(Point3f(float((2*j + i % 2)*squareSize),
                                          float(i*squareSize), 0));
        break;

      default:
        CV_Error(Error::StsBadArg, "Unknown pattern type\n");
    }
}

static bool runCalibration( vector<vector<Point2f> > imagePoints,
                    Size imageSize, Size boardSize, Pattern patternType,
                    float squareSize, float aspectRatio,
                    int flags, Mat& cameraMatrix, Mat& distCoeffs,
                    vector<Mat>& rvecs, vector<Mat>& tvecs,
                    vector<float>& reprojErrs,
                    double& totalAvgErr)
{
    cameraMatrix = Mat::eye(3, 3, CV_64F);
    if( flags & CALIB_FIX_ASPECT_RATIO )
        cameraMatrix.at<double>(0,0) = aspectRatio;

    distCoeffs = Mat::zeros(8, 1, CV_64F);

    vector<vector<Point3f> > objectPoints(1);
    calcChessboardCorners(boardSize, squareSize, objectPoints[0], patternType);

    objectPoints.resize(imagePoints.size(),objectPoints[0]);

    double rms = calibrateCamera(objectPoints, imagePoints, imageSize, cameraMatrix,
                    distCoeffs, rvecs, tvecs, flags|CALIB_FIX_K4|CALIB_FIX_K5);
                    ///*|CALIB_FIX_K3*/|CALIB_FIX_K4|CALIB_FIX_K5);
    printf("RMS error reported by calibrateCamera: %g\n", rms);

    bool ok = checkRange(cameraMatrix) && checkRange(distCoeffs);

    totalAvgErr = computeReprojectionErrors(objectPoints, imagePoints,
                rvecs, tvecs, cameraMatrix, distCoeffs, reprojErrs);

    return ok;
}


static void saveCameraParams( const string& filename,
                       Size imageSize, Size boardSize,
                       float squareSize, float aspectRatio, int flags,
                       const Mat& cameraMatrix, const Mat& distCoeffs,
                       const vector<Mat>& rvecs, const vector<Mat>& tvecs,
                       const vector<float>& reprojErrs,
                       const vector<vector<Point2f> >& imagePoints,
                       double totalAvgErr )
{
    FileStorage fs( filename, FileStorage::WRITE );

    time_t tt;
    time( &tt );
    struct tm *t2 = localtime( &tt );
    char buf[1024];
    strftime( buf, sizeof(buf)-1, "%c", t2 );

    fs << "calibration_time" << buf;

    if( !rvecs.empty() || !reprojErrs.empty() )
        fs << "nframes" << (int)std::max(rvecs.size(), reprojErrs.size());
    fs << "image_width" << imageSize.width;
    fs << "image_height" << imageSize.height;
    fs << "board_width" << boardSize.width;
    fs << "board_height" << boardSize.height;
    fs << "square_size" << squareSize;

    if( flags & CALIB_FIX_ASPECT_RATIO )
        fs << "aspectRatio" << aspectRatio;

    if( flags != 0 )
    {
        sprintf( buf, "flags: %s%s%s%s",
            flags & CALIB_USE_INTRINSIC_GUESS ? "+use_intrinsic_guess" : "",
            flags & CALIB_FIX_ASPECT_RATIO ? "+fix_aspectRatio" : "",
            flags & CALIB_FIX_PRINCIPAL_POINT ? "+fix_principal_point" : "",
            flags & CALIB_ZERO_TANGENT_DIST ? "+zero_tangent_dist" : "" );
        //cvWriteComment( *fs, buf, 0 );
    }

    fs << "flags" << flags;

    fs << "camera_matrix" << cameraMatrix;
    fs << "distortion_coefficients" << distCoeffs;

    fs << "avg_reprojection_error" << totalAvgErr;
    if( !reprojErrs.empty() )
        fs << "per_view_reprojection_errors" << Mat(reprojErrs);

    if( !rvecs.empty() && !tvecs.empty() )
    {
        CV_Assert(rvecs[0].type() == tvecs[0].type());
        Mat bigmat((int)rvecs.size(), 6, rvecs[0].type());
        for( int i = 0; i < (int)rvecs.size(); i++ )
        {
            Mat r = bigmat(Range(i, i+1), Range(0,3));
            Mat t = bigmat(Range(i, i+1), Range(3,6));

            CV_Assert(rvecs[i].rows == 3 && rvecs[i].cols == 1);
            CV_Assert(tvecs[i].rows == 3 && tvecs[i].cols == 1);
            //*.t() is MatExpr (not Mat) so we can use assignment operator
            r = rvecs[i].t();
            t = tvecs[i].t();
        }
        //cvWriteComment( *fs, "a set of 6-tuples (rotation vector + translation vector) for each view", 0 );
        fs << "extrinsic_parameters" << bigmat;
    }

    if( !imagePoints.empty() )
    {
        Mat imagePtMat((int)imagePoints.size(), (int)imagePoints[0].size(), CV_32FC2);
        for( int i = 0; i < (int)imagePoints.size(); i++ )
        {
            Mat r = imagePtMat.row(i).reshape(2, imagePtMat.cols);
            Mat imgpti(imagePoints[i]);
            imgpti.copyTo(r);
        }
        fs << "image_points" << imagePtMat;
    }
}

static bool readStringList( const string& filename, vector<string>& l )
{
    l.resize(0);
    FileStorage fs(filename, FileStorage::READ);
    if( !fs.isOpened() )
        return false;
    FileNode n = fs.getFirstTopLevelNode();
    if( n.type() != FileNode::SEQ )
        return false;
    FileNodeIterator it = n.begin(), it_end = n.end();
    for( ; it != it_end; ++it )
        l.push_back((string)*it);
    return true;
}


static bool runAndSave(const string& outputFilename,
                const vector<vector<Point2f> >& imagePoints,
                Size imageSize, Size boardSize, Pattern patternType, float squareSize,
                float aspectRatio, int flags, Mat& cameraMatrix,
                Mat& distCoeffs, bool writeExtrinsics, bool writePoints )
{
    vector<Mat> rvecs, tvecs;
    vector<float> reprojErrs;
    double totalAvgErr = 0;

    bool ok = runCalibration(imagePoints, imageSize, boardSize, patternType, squareSize,
                   aspectRatio, flags, cameraMatrix, distCoeffs,
                   rvecs, tvecs, reprojErrs, totalAvgErr);
    printf("%s. avg reprojection error = %.2f\n",
           ok ? "Calibration succeeded" : "Calibration failed",
           totalAvgErr);

    if( ok )
        saveCameraParams( outputFilename, imageSize,
                         boardSize, squareSize, aspectRatio,
                         flags, cameraMatrix, distCoeffs,
                         writeExtrinsics ? rvecs : vector<Mat>(),
                         writeExtrinsics ? tvecs : vector<Mat>(),
                         writeExtrinsics ? reprojErrs : vector<float>(),
                         writePoints ? imagePoints : vector<vector<Point2f> >(),
                         totalAvgErr );
    return ok;
}

int main(int argc, char **argv)
{
    //========================== pullImg
    char *camID = "rtsp://192.168.1.211:554/user=admin_password=tlJwpbo6_channel=1_stream=1.sdp?real_stream";
    int workingnum=0;
    pool_init (3);/*线程池中最多三个活动线程*/
    pool_add_worker (pullWebImg, camID);

    //========================== algo
    Size boardSize, imageSize;
    float squareSize, aspectRatio;
    Mat cameraMatrix, distCoeffs;
    string outputFilename;
    string inputFilename = "";

    int i, nframes;
    bool writeExtrinsics, writePoints;
    bool undistortImage = false;
    int flags = 0;
    bool flipVertical=1;
    bool showUndistorted;
    bool videofile;
    int delay;
    clock_t prevTimestamp = 0;
    int mode = DETECTION;
    int cameraId = 0;
    vector<vector<Point2f> > imagePoints;
    Pattern pattern = CHESSBOARD;

    boardSize.width = 8;
    boardSize.height = 5;
    string val = "chessboard";
    if( val == "circles" )
        pattern = CIRCLES_GRID;
    else if( val == "acircles" )
        pattern = ASYMMETRIC_CIRCLES_GRID;
    else if( val == "chessboard" )
        pattern = CHESSBOARD;
    else
        return fprintf( stderr, "Invalid pattern type: must be chessboard or circles\n" ), -1;
    squareSize = 1;
    nframes = 10;
    aspectRatio = 1;
    delay = 2000;
    // if (parser.has("a"))
    //     flags |= CALIB_FIX_ASPECT_RATIO;
    // if ( parser.has("zt") )
    //     flags |= CALIB_ZERO_TANGENT_DIST;
    // if ( parser.has("p") )
    //     flags |= CALIB_FIX_PRINCIPAL_POINT;

    outputFilename = "out_camera_data.yml";

    if ( squareSize <= 0 )
        return fprintf( stderr, "Invalid board square width\n" ), -1;
    if ( nframes <= 3 )
        return printf("Invalid number of images\n" ), -1;
    if ( aspectRatio <= 0 )
        return printf( "Invalid aspect ratio\n" ), -1;
    if ( delay <= 0 )
        return printf( "Invalid delay\n" ), -1;
    if ( boardSize.width <= 0 )
        return fprintf( stderr, "Invalid board width\n" ), -1;
    if ( boardSize.height <= 0 )
        return fprintf( stderr, "Invalid board height\n" ), -1;
    
    namedWindow( "Image View", 1 );

    for(int i = 0;;i++){
        Mat view, viewGray;
        getImg(view);
        if(view.empty())
            continue;
        if(flipVertical)
            flip(view, view, 0);

        bool blink = false;
       
        imageSize = view.size();
       
        vector<Point2f> pointbuf;
        cvtColor(view, viewGray, COLOR_BGR2GRAY);
       
        bool found;
        switch( pattern )
        {
         case CHESSBOARD:
             found = findChessboardCorners( view, boardSize, pointbuf,
                     CALIB_CB_ADAPTIVE_THRESH | CALIB_CB_FAST_CHECK | CALIB_CB_NORMALIZE_IMAGE);
             break;
         case CIRCLES_GRID:
             found = findCirclesGrid( view, boardSize, pointbuf );
             break;
         case ASYMMETRIC_CIRCLES_GRID:
             found = findCirclesGrid( view, boardSize, pointbuf, CALIB_CB_ASYMMETRIC_GRID );
             break;
         default:
             return fprintf( stderr, "Unknown pattern type\n" ), -1;
        }
       
        // improve the found corners' coordinate accuracy
        if( pattern == CHESSBOARD && found) cornerSubPix( viewGray, pointbuf, Size(11,11),
        Size(-1,-1), TermCriteria( TermCriteria::EPS+TermCriteria::COUNT, 30, 0.1 ));
       
        if( mode == CAPTURING && found &&
             (clock() - prevTimestamp > delay*1e-3*CLOCKS_PER_SEC) )
        {
            imagePoints.push_back(pointbuf);
            prevTimestamp = clock();
            blink = true;
        }
       
        if(found)
        	drawChessboardCorners( view, boardSize, Mat(pointbuf), found );
       
        string msg = mode == CAPTURING ? "100/100" :
         mode == CALIBRATED ? "Calibrated" : "Press 'g' to start";
        int baseLine = 0;
        Size textSize = getTextSize(msg, 1, 1, 1, &baseLine);
        Point textOrigin(view.cols - 2*textSize.width - 10, view.rows - 2*baseLine - 10);
       
        if( mode == CAPTURING ){
        	if(undistortImage)
        		msg = format( "%d/%d Undist", (int)imagePoints.size(), nframes );
        	else
        		msg = format( "%d/%d", (int)imagePoints.size(), nframes );
        }
       
        putText( view, msg, textOrigin, 1, 1,
             mode != CALIBRATED ? Scalar(0,0,255) : Scalar(0,255,0));
       
        if( blink )
        	bitwise_not(view, view);
       
        if( mode == CALIBRATED && undistortImage ){
            Mat temp = view.clone();
            undistort(temp, view, cameraMatrix, distCoeffs);
        }
       
        imshow("Image View", view);
        char key = (char)waitKey(50);
       
        if( key == 27 )
        	break;

        if( key == 'w' && mode == CALIBRATED && undistortImage )
        	cv::imwrite("undistortImage.jpeg", view);
        if( key == 'w' && !undistortImage )
            cv::imwrite("distortImage.jpeg", view);
        if( key == 'u' && mode == CALIBRATED )
        	undistortImage = !undistortImage;
       
        if( key == 'g' ){
        	mode = CAPTURING;
        	imagePoints.clear();
        }
       
        if( mode == CAPTURING && imagePoints.size() >= (unsigned)nframes ){
        	if( runAndSave(outputFilename, imagePoints, imageSize,
        		boardSize, pattern, squareSize, aspectRatio,
        		flags, cameraMatrix, distCoeffs,
        		writeExtrinsics, writePoints))
        		mode = CALIBRATED;
        	else
        		mode = DETECTION;
        }
    }

    //释放线程
    pool_destroy ();

}
