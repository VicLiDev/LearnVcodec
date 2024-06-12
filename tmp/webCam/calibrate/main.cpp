//原文：https://blog.csdn.net/guyuealian/article/details/79607568 
//命令行编译指令:g++ -o webCali -w main.cpp webcamdata.cpp -lpthread $(pkg-config --libs --cflags opencv libavcodec libavdevice libavfilter libavformat libavutil)
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

using namespace std;
using namespace cv;

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

//============================================================================================= algo1 begin

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

//============================================================================================= algo1 end


int main(int argc, char **argv)
{
	//========================== pullImg
	// 需要添加更多摄像头的时候只需要在这里加入链接，然后把他创建的 WebCamData 对象压到栈里就可以了，然后就可以在后边写算法了
    std::vector<WebCamData> m_webCamDataV;
    std::string camID1 = "rtsp://192.168.1.211:554/user=admin_password=tlJwpbo6_channel=1_stream=1.sdp?real_stream";
    // std::string camID2 = "rtsp://192.168.1.167:554/user=admin_password=tlJwpbo6_channel=1_stream=1.sdp?real_stream";
    m_webCamDataV.push_back(WebCamData(camID1));
    // m_webCamDataV.push_back(WebCamData(camID2));

    pool_init (3);/*线程池中最多三个活动线程*/
    for(int i = 0; i < m_webCamDataV.size(); i++){
        pool_add_worker(pullWebImg, &m_webCamDataV[i]);
    }
    //========================== pullImg end

    //============================================================================================= algo2 begin
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
    //============================================================================================= algo2 end

    std::vector<cv::Mat> views(m_webCamDataV.size());
    for(int i = 0;;i++){
    	// for(int i = 0; i < m_webCamDataV.size(); i++){
    	// 	m_webCamDataV[i].getImg(views[i]);
    	// }

    	// for(int i = 0; i < views.size(); i++){
    	// 	if(views[i].empty()){
    	// 		std::cout << "img " << std::to_string(i) << " is empty!!" << std::endl;
    	// 		continue;
    	// 	}else{
    	// 		// 工作时间长了以后画面显示会卡住，但是通过 imwrite 可以看到，获取图像的线程是在正常工作的
    	// 		cv::imshow("getImg" + std::to_string(i), views[i]);
    	// 		cv::imwrite("getImg" + std::to_string(i) + ".jpg", views[i]);
    	// 	}
    	// }
    	// // 延时不能太短，不然加载图像的线程无法获取互斥锁，如果这里有耗时比较久的算法可以减少延时时长
    	// char key = (char)cv::waitKey(1);
    	// if( key == 27 )
    	// 	break;

    	//===================================== 算法
    	// ...
    	Mat view, viewGray;
        m_webCamDataV[0].getImg(view);
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
    	//===================================== 算法 end
    }

    //释放线程
    pool_destroy ();

}
