/*************************************************************************
    > File Name: opencma.cpp
    > Author: LiHongjin
    > Mail: 872648180@qq.com
    > Created Time: Tue 08 Jul 2025 08:24:32 PM CST
 ************************************************************************/

/* build: g++ opencma.cpp -o opencma `pkg-config --cflags --libs opencv4` */

#include <opencv2/opencv.hpp>
#include <iostream>

int main()
{
    int camera_available = -1;

    // 尝试检查前两个摄像头（/dev/video0 和 /dev/video1）
    for (int i = 0; i < 2; ++i) {
        cv::VideoCapture cap(i);

        if (cap.isOpened()) {
            cv::Mat frame;
            bool ret = cap.read(frame);

            if (ret && !frame.empty()) {
                std::cout << "✅ Camera " << i << " Available" << std::endl;
                camera_available = i;
            } else {
                std::cout << "⚠️ Camera " << i << " is open but cannot read image" << std::endl;
            }

            cap.release();
        } else {
            std::cout << "❌ Camera " << i << " cannot be opened" << std::endl;
        }
    }

    if (camera_available < 0) {
        std::cerr << "No available camera found." << std::endl;
        return 0;
    }

    // 打开找到的可用摄像头
    cv::VideoCapture cap(camera_available);
    if (!cap.isOpened()) {
        std::cerr << "Cannot open camera." << std::endl;
        return -1;
    }

    // 设置分辨率（可选）
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    cv::Mat frame;
    while (true) {
        bool ret = cap.read(frame);

        if (!ret || frame.empty()) {
            std::cerr << "Can't receive frame. Exiting..." << std::endl;
            break;
        }

        cv::imshow("Camera", frame);

        if (cv::waitKey(1) == 'q') {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();
    return 0;
}

