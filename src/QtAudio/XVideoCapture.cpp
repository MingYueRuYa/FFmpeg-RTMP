#include "XVideoCapture.h"

#include <opencv2/highgui.hpp>

#include <iostream>

using std::cout;
using std::endl;

#pragma comment(lib, "opencv_world331.lib")

class CXVideoCapture : public XVideoCapture {
public:
    cv::VideoCapture camera;

    void run() {
        cv::Mat frame;

        while (! isExit) {
 		if (!camera.read(frame))
			{
				msleep(1);
				continue;
			}
			if (frame.empty())
			{
				msleep(1);
				continue;
			}/*
			imshow("v", frame);
			waitKey(1);*/
			//确保数据是连续的 
			fmutex.lock();
			for (int i = 0; i < filters.size(); i++)
			{
				cv::Mat des;
				filters[i]->Filter(&frame, &des);
				frame = des;
			}
			fmutex.unlock();

			XData d((char*)frame.data, frame.cols*frame.rows*frame.elemSize(),GetCurTime());
			Push(d);
        }
    }

    bool Init(int cameraIndex = 0) {
        camera.open(cameraIndex);
        if (! camera.isOpened()) { 
            cout << "camera open failed" << endl;
            return false; 
        }

        cout << cameraIndex << " camera open success" << endl;
		width   = camera.get(cv::CAP_PROP_FRAME_WIDTH);
		height  = camera.get(cv::CAP_PROP_FRAME_HEIGHT);
		fps     = camera.get(cv::CAP_PROP_FPS);
		if (fps == 0) fps = 25;
		return true;
    }

    bool Init(const char *url) {
		camera.open(url);
		if (!camera.isOpened())
		{
			cout << "cam open failed!" << endl;
			return false;
		}
		cout << url << " cam open success" << endl;
		width   = camera.get(cv::CAP_PROP_FRAME_WIDTH);
		height  = camera.get(cv::CAP_PROP_FRAME_HEIGHT);
		fps     = camera.get(cv::CAP_PROP_FPS);
		if (fps == 0) fps = 25;
		return true;
	}

    void Stop() {
        XDataThread::Stop();

        if (camera.isOpened()) {
            camera.release();
        }
    }
};

XVideoCapture *XVideoCapture::Get(unsigned char index) {
	static CXVideoCapture xc[255];
	return &xc[index];
}

XVideoCapture::XVideoCapture() {
}

XVideoCapture::~XVideoCapture() {
}