#include <iostream>
// #include <string>
#include <opencv2/opencv.hpp>

class WebCamData{
public:
	WebCamData(std::string str);
	~WebCamData();

	int loadImg(cv::Mat &img);

	// 在调用该函数之后要注意判断数据是否为空
	// 例如： getImg(view);
	//       if(view.empty())
	//            continue;
	int getImg(cv::Mat &getImg);

	std::string webCamID;
private:
	cv::Mat webImg;
	pthread_mutex_t m_mutex;
};