//网上搜的小程序,不属于下面两种方法
bool YV12ToBGR24_OpenCV(unsigned char* pYUV,unsigned char* pBGR24,int width,int height)
{
    if (width < 1 || height < 1 || pYUV == NULL || pBGR24 == NULL)
        return false;
    Mat dst(height,width,CV_8UC3,pBGR24);
    Mat src(height + height/2,width,CV_8UC1,pYUV);
    cvtColor(src,dst,CV_YUV2BGR_YV12);
    return true;
}

//======================================================================================
//OpenCV 自带转换函数法 cvCvtColor
//测试文件名：football_cif.yuv， FOOTBALL_352x288_30_orig_01.yuv
//图像大小：352*288
//这两个文件的数据存储格式为：YUV，即先存储所有的Y，再存储所有的U，最后存储所有的V（非交叉存储）
//#include <iostream>
#include <highgui.h>
#include <cv.h>
#include <fstream>
#include <sstream>
using namespace std;
#define FCount 10
#define ISizeX 352	//图像宽度
#define ISizeY 288	//图像高度

unsigned char Y[FCount][ISizeY][ISizeX];   
unsigned char U[FCount][ISizeY/2][ISizeX/2];	
unsigned char V[FCount][ISizeY/2][ISizeX/2];

// 将图片文件写入
void FileWriteFrames()
{
	char *filename = "football_cif.yuv";
	ifstream readMe(filename, ios::in | ios::binary);  // 打开并读yuv数据
	IplImage *image, *rgbimg, *yimg, *uimg, *vimg, *uuimg, *vvimg;
	cvNamedWindow("yuv",CV_WINDOW_AUTOSIZE);
	rgbimg = cvCreateImage(cvSize(ISizeX, ISizeY), IPL_DEPTH_8U, 3);
	image = cvCreateImage(cvSize(ISizeX, ISizeY), IPL_DEPTH_8U, 3);
    
	yimg = cvCreateImageHeader(cvSize(ISizeX, ISizeY), IPL_DEPTH_8U, 1);    // 亮度分量
	uimg = cvCreateImageHeader(cvSize(ISizeX/2, ISizeY/2), IPL_DEPTH_8U, 1);  // 这两个都是色度分量
	vimg = cvCreateImageHeader(cvSize(ISizeX/2, ISizeY/2), IPL_DEPTH_8U, 1);
    
	uuimg = cvCreateImage(cvSize(ISizeX, ISizeY), IPL_DEPTH_8U, 1);
	vvimg = cvCreateImage(cvSize(ISizeX, ISizeY), IPL_DEPTH_8U, 1);
	int nframes;
	for(nframes = 0; nframes < FCount; nframes ++)
	{
		char nframesstr[20];

		readMe.read((char*)Y[nframes],ISizeX*ISizeY);	//读取Y分量
		readMe.read((char*)U[nframes],ISizeX/2*ISizeY/2);//读取U分量
		readMe.read((char*)V[nframes],ISizeX/2*ISizeY/2);//读取V分量

		cvSetData(yimg,Y[nframes],ISizeX);
		cvSetData(uimg,U[nframes], ISizeX/2);
		cvSetData(vimg,V[nframes], ISizeX/2);
		
		
		cvResize(uimg,uuimg, CV_INTER_LINEAR);
		cvResize(vimg,vvimg, CV_INTER_LINEAR);
		cvMerge(yimg,uuimg,vvimg,NULL,image);   // 合并单通道为三通道
		cvCvtColor(image,rgbimg,CV_YCrCb2RGB);  //	YUV转换为RGB
		
		stringstream ss;  // 类型转换统一转换为char* 类型
		ss << nframes;
		ss << ".jpg" ;
		ss >> nframesstr;
		cvShowImage("yuv", rgbimg);
		cvSaveImage(nframesstr,rgbimg);
		int c = cvWaitKey(300);
		if((char)c == 27)
		{
			break;
		}
	}
	readMe.close();
	cvReleaseImage(&uuimg);
    <span style="white-space:pre">	</span>cvReleaseImage(&vvimg);
	cvReleaseImageHeader(&yimg);
	cvReleaseImageHeader(&uimg);
	cvReleaseImageHeader(&vimg);
	cvReleaseImage(&image);
	cvDestroyWindow("yuv");	
}


int main()
{
	FileWriteFrames();
	return 0;
}

//======================================================================================
//公式法
//测试数据：test1.bmp， test2.bmp
//数据存储格式：UYVY（交叉存储）
//图像大小：1024*480
#include <iostream>
#include <highgui.h>
#include <cv.h>
#include <fstream>
#include <sstream>

using namespace std;

#define width 1024 //图像宽度
#define height 480	//图像高度

unsigned char *yuvtorgb24(char *bufyuv, int w, int h)
{
	int r1,g1,b1,r2,g2,b2;
	int y1,u,v,y2;
	unsigned char *ps=(unsigned char *)bufyuv;
	unsigned char *pd;
	int len;
	
	if ((pd = (unsigned char *)malloc (w * h * 3)) == NULL)
		return NULL;
	
	len=w*h;
	while(len>0)
	{
		len-=2;
		u=*ps++;	
		y1 =*ps++;
		v=*ps++;	
		y2 =*ps++;	
		r1=(10000*y1+14075*(v-128))/10000;
		g1=(10000*y1-3455*(u-128)-7169*(v-128))/10000;
		b1=(10000*y1+17990*(u-128))/10000;
		r2=(10000*y2+14075*(v-128))/10000;
		g2=(10000*y2-3455*(u-128)-7169*(v-128))/10000;
		b2=(10000*y2+17990*(u-128))/10000;
		
		if(r1>255)r1=255;if(r1<0)r1=0;
		if(g1>255)g1=255;if(g1<0)g1=0;
		if(b1>255)b1=255;if(b1<0)b1=0;
		if(r2>255)r2=255;if(r2<0)r2=0;
		if(g2>255)g2=255;if(g2<0)g2=0;
		if(b2>255)b2=255;if(b2<0)b2=0;
		
		*pd++=b1;*pd++=g1;*pd++=r1;
		*pd++=b2;*pd++=g2;*pd++=r2;
	}


	return pd-(w*h*3);
}


int main()
{
	
	int i, j;
	unsigned char *p;
	char recvBuf[width*height*2];
	IplImage *srcImg;
	char *filename = "test2.bmp";

	srcImg = cvCreateImage(cvSize(width, height), 8, 3);
	cvZero(srcImg);

	ifstream fin(filename, ios::in | ios::binary);  // 打开并读yuv数据  
	fin.read(recvBuf, width*height*2*sizeof(char)); 
/*	查看数据
	for(i=0; i<500; i++)
	{
		unsigned char tmp = (unsigned char)recvBuf[i];
		cout<< (unsigned short)tmp<<endl;
	}
*/
	//	yuv格式转换为rgb格式
	p = yuvtorgb24(recvBuf, 1024, 480);
	//	图像颜色赋值
	for(i=0; i<height; i++)
	{
		for(j=0; j<width; j++)
		{
			((uchar *)(srcImg->imageData + i*srcImg->widthStep))[j*srcImg->nChannels + 0] = p[(i*width+j)*3 + 0];
			((uchar *)(srcImg->imageData + i*srcImg->widthStep))[j*srcImg->nChannels + 1] = p[(i*width+j)*3 + 1];
			((uchar *)(srcImg->imageData + i*srcImg->widthStep))[j*srcImg->nChannels + 2] = p[(i*width+j)*3 + 2];
		}
	}
	free(p);
	cvSaveImage("mytest2.bmp", srcImg);
	cvNamedWindow("YUV2RGB", 1);
	cvShowImage("YUV2RGB", srcImg);
	cvWaitKey(0);
	cvReleaseImage(&srcImg);
	return 0;
}
