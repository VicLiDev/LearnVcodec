#include <iostream>
#include <opencv/highgui.h>

using namespace std;
using namespace cv;  

class  ConvertYuvForCV  
{  
  public:  
       ConvertYuvForCV(const int&iWidth,const int &Height,const string &strCvWindowName);  
        ~ConvertYuvForCV();  
        bool CvWindowShow();  
        void YUYV422ToIplImage(const unsigned char *pYUYV422,const int &iLen);  
  private:  
         void YUVToRGB(const int &iY, const int & iU, const int & iV, int &iR, int &iG, int &iB);  
         void ConvertYUYV422ToRGB(const unsigned char* pYUYV422, const int &iLen,unsigned char *pRGB);  
         void ConvertYUYV422ToBGR(const unsigned char* pYUYV422, const int &iLen,unsigned char *pRGB);  
  private:  
         int m_iWidth;  
         int m_Height;  
         IplImage *m_IplImage;  
         unsigned char *m_pBGR;  
         std::string  m_strCvWindowName;  
};  
  
ConvertYuvForCV::ConvertYuvForCV(const int&iWidth,const int &iHeight ,const string &strCvWindowName)  
{  
    m_iWidth = iWidth;  
    m_Height = iHeight;  
    m_IplImage = cvCreateImage(cvSize(iWidth, iHeight),IPL_DEPTH_8U, 3);  
    m_pBGR     = new unsigned char[iWidth*iHeight*3];  
    m_strCvWindowName = strCvWindowName;  
    if(!m_strCvWindowName.empty()){  
        cvNamedWindow(m_strCvWindowName.c_str());  
    }  
}  
ConvertYuvForCV::~ConvertYuvForCV()  
{  
    cvReleaseImage(&m_IplImage);  
    delete[]m_pBGR;  
    if(!m_strCvWindowName.empty()){  
        cvDestroyWindow(m_strCvWindowName.c_str());  
    }  
}  
void ConvertYuvForCV::YUYV422ToIplImage(const unsigned char *pYUYV422,const int &iLen)  
{  
    ConvertYUYV422ToBGR(pYUYV422,iLen,m_pBGR);  
    cvSetData(m_IplImage,m_pBGR,m_iWidth*3);
}  
void ConvertYuvForCV::YUVToRGB(const int &iY, const int & iU, const int & iV, int &iR, int &iG, int &iB)  
{  
    assert( &iR != NULL && &iG != NULL && &iB != NULL);  
  
    iR = iY + 1.13983 * (iV - 128);  
    iG = iY - 0.39465 * (iU - 128) - 0.58060 * (iV - 128);  
    iB = iY + 2.03211 * (iU - 128);  
  
    iR = iR > 255 ? 255 : iR;  
    iR = iR < 0   ? 0   : iR;  
  
    iG = iG > 255 ? 255 : iG;  
    iG = iG < 0   ? 0   : iG;  
  
    iB = iB > 255 ? 255 : iB;  
    iB = iB < 0   ? 0   : iB;  
}  
void ConvertYuvForCV::ConvertYUYV422ToRGB(const unsigned char* pYUYV422, const int &iLen,unsigned char *pRGB)  
{  
    //YUYV422 640*480*2     TO  BGR 640*480*3  
    int iR, iG, iB;  
    int iY0, iY1, iU, iV;  
    int i = 0;  
    int j = 0;  
    for(i = 0; i < iLen; i += 4)  
    {  
        iY0 = pYUYV422[i + 0];  
        iU  = pYUYV422[i + 1];  
        iY1 = pYUYV422[i + 2];  
        iV  = pYUYV422[i + 3];  
        YUVToRGB(iY0, iU, iV, iR, iG, iB);  
        pRGB[j++] = iR;  
        pRGB[j++] = iG;  
        pRGB[j++] = iB;  
        YUVToRGB(iY1, iU, iV, iR, iG, iB);  
        pRGB[j++] = iR;  
        pRGB[j++] = iG;  
        pRGB[j++] = iB;  
    }  
    pRGB[j] = '\0';  
}  
void ConvertYuvForCV::ConvertYUYV422ToBGR(const unsigned char* pYUYV422, const int &iLen,unsigned char *pBGR)  
{  
    //YUYV422 640*480*2     TO  BGR 640*480*3  
    int iR, iG, iB;  
    int iY0, iY1, iU, iV;  
    int i = 0;  
    int j = 0;  
    for(i = 0; i < iLen; i += 4)  
    {  
        //其排列方式是y0 u y1 v  
        //直接提取出来y、u、v三个分量，然后使用公式转成RGB即可  
        //因为两个y共享一对uv。故y0 u y1 v能提取出两组(y, u, v)  
        iY0 = pYUYV422[i + 0];  
        iU  = pYUYV422[i + 1];  
        iY1 = pYUYV422[i + 2];  
        iV  = pYUYV422[i + 3];  
        YUVToRGB(iY0, iU, iV, iR, iG, iB);  
        pBGR[j++] = iB;  
        pBGR[j++] = iG;  
        pBGR[j++] = iR;  
        YUVToRGB(iY1, iU, iV, iR, iG, iB);  
        pBGR[j++] = iB;  
        pBGR[j++] = iG;  
        pBGR[j++] = iR;  
    }  
    pBGR[j] = '\0';  
}  
bool  ConvertYuvForCV::CvWindowShow()  
{  
    if(!m_strCvWindowName.empty())  
    {  
        cvShowImage(m_strCvWindowName.c_str(), m_IplImage);  
		cvSaveImage("test.jpg", m_IplImage);
        cvWaitKey(33);  
    }  
    else  
    {  
        std::cout<<"Have no Available CvWindow "<<std::endl;  
        return false;  
    }  
    return true;  
}  
int main()  
{  
    const char* pFilename = "test.yuv";  
    FILE* pFin = fopen(pFilename, "rb");  
    if( pFin == NULL )  
    {  
        printf("can't open the file\n");  
        return -1;  
    }  
    int iWidth = 640;  
    int iHeight = 480;  
    int iFrameSize = iWidth * iHeight * 2;  
    unsigned char* pYUYV422Buff = new unsigned char[iFrameSize];  
    string strCvWindowName("FUCK");  
    ConvertYuvForCV oConvertYuvForCV(iWidth,iHeight,strCvWindowName);  
    while( 1 )  
    {  
        int iRet = fread(pYUYV422Buff, 1, iFrameSize, pFin);  
        if( iRet != iFrameSize )  
        {  
           break;  
        }  
        oConvertYuvForCV.YUYV422ToIplImage(pYUYV422Buff,iFrameSize);  
        oConvertYuvForCV.CvWindowShow();  
    }  
    delete[]pYUYV422Buff;  
    return 0;  
}  
