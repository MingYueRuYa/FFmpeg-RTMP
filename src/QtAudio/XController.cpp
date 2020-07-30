#include "XController.h"
#include "XVideoCapture.h"
#include "XAudioRecord.h"
#include "XMediaEncode.h"
#include "XRtmp.h"
#include <iostream>

using namespace std;

void XController::run()
{
	long long beginTime = GetCurTime();
	while (!isExit)
	{
		//一次读取一帧音频
		XData ad = XAudioRecord::Get()->Pop();
		XData vd = XVideoCapture::Get()->Pop();
		if (ad.size<=0 && vd.size<=0)
		{
			msleep(1);
			continue;
		}
		//处理音频
		if (ad.size > 0)
		{
			ad.pts = ad.pts - beginTime;

			//重采样源数据
			XData pcm = XMediaEncode::Get()->Resample(ad);
			ad.Drop();
			XData pkt = XMediaEncode::Get()->EncodeAudio(pcm);
			if (pkt.size > 0)
			{
				////推流
				if (XRtmp::Get()->SendFrame(pkt,aindex))
				{
					cout << "#" << flush;
				}
			}
		}
		//处理视频
		if (vd.size > 0)
		{
			vd.pts = vd.pts - beginTime;
			XData yuv = XMediaEncode::Get()->RGBToYUV(vd);
			vd.Drop();
			XData pkt = XMediaEncode::Get()->EncodeVideo(yuv);
			if (pkt.size> 0)
			{
				////推流
				if (XRtmp::Get()->SendFrame(pkt,vindex))
				{
					cout << "@" << flush;
				}
			}
		}
	}
}


//设定美颜参数
bool XController::Set(std::string key, double val)
{
	XFilter::Get()->Set(key, val);
	return true;
}
bool XController::Start()
{
	///1 设置磨皮过滤器
	XVideoCapture::Get()->AddFilter(XFilter::Get());
	cout << "1 设置磨皮过滤器" << endl;
	///2 打开相机
	if (camIndex >= 0)
	{
		if (!XVideoCapture::Get()->Init(camIndex))
		{
			err = "2打开系统相机失败";
			cout << err << endl;
			return false;
		}
	}
	else if (!inUrl.empty())
	{
		if (!XVideoCapture::Get()->Init(inUrl.c_str()))
		{
			err = "2打开";
			err += inUrl;
			err += "相机失败";
			cout << err << endl;
			return false;
		}
	}
	else
	{
		err = "2请设置相机参数";
		cout << err << endl;
		return false;
	}
	cout << "2相机打开成功" << endl;
	
	///3 qt音频开始录制
	if (!XAudioRecord::Get()->Init())
	{
		err = "3录音设备打开失败";
		cout << err << endl;
		return false;
	}
	cout << "3录音设备打开成功" << endl;

	///11 启动音视频录制线程
	XAudioRecord::Get()->Start();
	XVideoCapture::Get()->Start();

	///音视频编码类
	///4 初始化格式转换上下文
	///初始化输出的数据结构
	XMediaEncode::Get()->in_width = XVideoCapture::Get()->width;
	XMediaEncode::Get()->in_height = XVideoCapture::Get()->height;
	XMediaEncode::Get()->out_width = XVideoCapture::Get()->width;
	XMediaEncode::Get()->out_height = XVideoCapture::Get()->height;
	if (!XMediaEncode::Get()->InitScale())
	{
		err = "4视频像素格式转换打开失败!";
		cout << err << endl;
		return false;
	}
	cout << "4视频像素格式转换打开成功!" << endl;
	///5 音频重采样 上下文初始化
	XMediaEncode::Get()->channels = XAudioRecord::Get()->channels;
	XMediaEncode::Get()->nb_sample = XAudioRecord::Get()->nbSamples;
	XMediaEncode::Get()->sample_rate = XAudioRecord::Get()->sampleRate;
	if (!XMediaEncode::Get()->InitResample())
	{
		err = "5音频重采样上下文初始化失败!";
		cout << err << endl;
		return false;
	}
	cout << "5音频重采样上下文初始化成功!" << endl;
	///6初始化音频编码器
	if (!XMediaEncode::Get()->InitAudioCode())
	{
		err = "6初始化音频编码器失败!";
		cout << err << endl;
		return false;
	}
	///7初始化视频编码器
	if (!XMediaEncode::Get()->InitVideoCodec())
	{
		err = "7初始化视频频编码器失败!";
		cout << err << endl;
		return false;
	}
	cout << "7音视频编码器初始化成功!" << endl;
	/// 8创建输出封装器上下文
	if (!XRtmp::Get()->Init(outUrl.c_str()))
	{
		err = "8创建输出封装器上下文失败!";
		cout << err << endl;
		return false;
	}
	cout << "8创建输出封装器上下文成功!" << endl;

	///9 添加音视频流
	vindex = XRtmp::Get()->AddStream(XMediaEncode::Get()->vc);
	aindex = XRtmp::Get()->AddStream(XMediaEncode::Get()->ac);
	if (vindex < 0 || aindex < 0)
	{
		err = "9添加音视频流失败!";
		cout << err << endl;
		return false;
	}
	cout << "9添加音视频流成功!" << endl;

	///10 发送封装头
	if (!XRtmp::Get()->SendHead())
	{
		err = "10 发送封装头失败!";
		cout << err << endl;
		return false;
	}
	XAudioRecord::Get()->Clear();
	XVideoCapture::Get()->Clear();
	XDataThread::Start();
	return true;
}
void XController::Stop()
{
	XDataThread::Stop();
	XAudioRecord::Get()->Stop();
	XVideoCapture::Get()->Stop();
	XMediaEncode::Get()->Close();
	XRtmp::Get()->Close();
	camIndex = -1;
	inUrl = "";
	return;
}

XController::XController()
{
}


XController::~XController()
{
}
