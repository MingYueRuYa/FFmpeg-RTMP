#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;

using std::cout;
using std::endl;

#pragma comment(lib, "opencv_world331.lib")

void showimage()
{
    Mat image = imread("wechat_web.png");
    namedWindow("img");
    
    imshow("img", image);
    waitKey(0);
}

void padding_rgb()
{
    Mat mat(800, 600, CV_8UC3);

    // 元素字节数大小
    int es = mat.elemSize();
    int size = mat.rows*mat.cols*es;

    // 地址遍历连续的Mat
    for (int i = 0; i < size; i += es) {
        mat.data[i] = 255; // B
        mat.data[i+1] = 100; // G
        mat.data[i+2] = 100; // R
    }

    namedWindow("mat");
    imshow("mat", mat);
    waitKey(0);
}

void open_usb_camera()
{
    // 表示第一摄像头设备
    VideoCapture vediocapture(0);
    
    if (! vediocapture.isOpened()) {
        cout << "vediocapture open error" << endl;
        return ;
    }

    Mat frame;
    while (vediocapture.read(frame)) {
        imshow("video-demo", frame);

        // 按下ESC键表示退出
        char c = waitKey(30);
        if (27 == c) { break; }
    }

    vediocapture.release();
    destroyAllWindows();
}

int main(int argc, char *argv[])
{
    // showimage();

    // padding_rgb();

    open_usb_camera();

    return 0;
}