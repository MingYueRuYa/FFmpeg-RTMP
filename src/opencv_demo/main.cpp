#include "common.h"

#include <iostream>
#include <memory>
#include <functional>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
}

using namespace cv;

using std::cout;
using std::endl;
using std::cerr;
using std::flush;
using std::unique_ptr;

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
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
    Mat mat(200, 500, CV_8UC3);

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

void init_av()
{
    // 注册所有的编解码器
    avcodec_register_all();

    // 注册所有的封装器
    av_register_all();

    // 注册所有网络协议
    avformat_network_init();
}

void opencv_rtmp() {
    char *out_url = "rtmp://192.168.26.31/live";
    
    init_av();

    // 输出的数据结构
    AVFrame *yuv = NULL;

    unique_ptr<VideoCapture, std::function<void(VideoCapture *)>> video_ptr(
        new VideoCapture(), [](VideoCapture *captrue) { 
        if (captrue->isOpened()) { captrue->release(); }
        delete captrue;
    });

    Mat frame;

    // 1.使用opencv 打开usb 摄像头
    video_ptr->open(0);

    if (! video_ptr->isOpened()) { 
        cout << "camera open usb camera error" << endl;
        return;
    }

    cout << "open usb camera successful." << endl;

    int width   = video_ptr->get(CAP_PROP_FRAME_WIDTH);
    int height  = video_ptr->get(CAP_PROP_FRAME_HEIGHT);
    int fps     = video_ptr->get(CAP_PROP_FPS);

    // 如果fps为0，这里就设置25。因为在fps=0时，调用avcodec_open2返回-22,
    // 参数不合法
    if (0 == fps) { fps = 25; }

    // 2.初始化格式转换上下文
    SwsContext *sws_context = NULL;
    sws_context = sws_getCachedContext(sws_context,
                    width, height, AV_PIX_FMT_BGR24,    // 源格式
                    width, height, AV_PIX_FMT_YUV420P,  // 目标格式
                    SWS_BICUBIC,    // 尺寸变化使用算法
                    0, 0, 0);

    if (NULL == sws_context) { 
        cout << "sws_getCachedContext error" << endl;
        return;
    }

    unique_ptr<SwsContext, std::function<void(SwsContext *)>> swscontext_tr
        (sws_context, [](SwsContext *swscontext) {
            sws_freeContext(swscontext);
            swscontext = nullptr;
    });

    // 3.初始化输出的数据结构
    yuv         = av_frame_alloc();
    yuv->format = AV_PIX_FMT_YUV420P;
    yuv->width  = width;
    yuv->height = height;
    yuv->pts    = 0;

    // 分配yuv空间
    int ret_code = av_frame_get_buffer(yuv, 32);
    if (0 != ret_code) {
        av_error(ret_code);
        return;
    }

    // 4.初始化编码上下文
    // 4.1找到编码器
    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (NULL == codec) { 
        cout << "Can't find h264 encoder." << endl; 
        return;
    }

    // 4.2创建编码器上下文
    AVCodecContext *codec_context = avcodec_alloc_context3(codec);
    if (NULL == codec_context) { 
       cout << "avcodec_alloc_context3 failed." << endl;
       return;
    }

    unique_ptr<AVCodecContext, std::function<void(AVCodecContext *)>> codec_context_ptr
        (codec_context, [](AVCodecContext *context) {
            avcodec_free_context(&context);
    });

    // 4.3配置编码器参数
    // vc->flags           |= AV_CODEC_FLAG_GLOBAL_HEADER;
    codec_context->codec_id        = codec->id;
    codec_context->thread_count    = 8;

    // 压缩后每秒视频的bit流 50k
    codec_context->bit_rate = 50*1024*8;
    codec_context->width = width;
    codec_context->height = height;
    codec_context->time_base = {1, fps};
    codec_context->framerate = {fps, 1};

    // 画面组的大小，多少帧一个关键帧
    codec_context->gop_size = 50;
    codec_context->max_b_frames = 0;
    codec_context->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_context->qmin = 10;
    codec_context->qmax = 51;

    // 4.4打开编码器上下文
    ret_code = avcodec_open2(codec_context, 0, 0);
    if (0 != ret_code) { 
        av_error(ret_code);
        return;
    }
    cout << "avcodec_open2 success!" << endl;

    // 5.输出封装器和视频流配置
    // 5.1创建输出封装器上下文
    // rtmp flv封装器
    AVFormatContext *format_context = nullptr;
    ret_code = avformat_alloc_output_context2(&format_context, 0, "flv", out_url);
    if (0 != ret_code) { 
        av_error(ret_code);
        return;
    }

    unique_ptr<AVFormatContext, std::function<void(AVFormatContext *)>> format_context_ptr
        (format_context, [](AVFormatContext *context) {
        avio_closep(&context->pb); 
    });

    // 5.2添加视频流
    AVStream *vs = avformat_new_stream(format_context, NULL);
    if (NULL == vs) { 
        cout << "avformat_new_stream failed." << endl;
        return;
    }

    vs->codecpar->codec_tag = 0;
    // 从编码器复制参数
    avcodec_parameters_from_context(vs->codecpar, codec_context);
    av_dump_format(format_context, 0, out_url, 1);

    // 打开rtmp 的网络输出IO
    ret_code = avio_open(&format_context->pb, out_url, AVIO_FLAG_WRITE);
    if (0 != ret_code) { 
        av_error(ret_code);
        return;
    }

    // 写入封装头
    ret_code = avformat_write_header(format_context, NULL);
    if (0 != ret_code) { 
        av_error(ret_code);
        return;
    }

    AVPacket pack;
    memset(&pack, 0, sizeof(pack));
    int vpts = 0;
    for (;;) {
        // 读取rtsp视频帧，解码视频帧
        if (! video_ptr->grab()) { continue; }

        // yuv转换为rgb
        if (! video_ptr->retrieve(frame)) { continue; }

        imshow("video", frame);
        waitKey(1);

        // rgb to yuv
        uint8_t *in_data[AV_NUM_DATA_POINTERS] = {0};
        in_data[0] = frame.data;
        int in_size[AV_NUM_DATA_POINTERS] = {0};
        // 一行（宽）数据的字节数
        in_size[0] = frame.cols * frame.elemSize();
        int h = sws_scale(sws_context, in_data, in_size, 0, frame.rows,
                            yuv->data, yuv->linesize);
        if (h <= 0) { continue; }

        // h264编码
        yuv->pts = vpts;
        vpts++;

        ret_code = avcodec_send_frame(codec_context, yuv);
        if (0 != ret_code) { continue; }

        ret_code = avcodec_receive_packet(codec_context, &pack);
        
        if (0 != ret_code || pack.buf > 0) {
            //TODO something
        } else { continue; }

        // 推流
        pack.pts = av_rescale_q(pack.pts, codec_context->time_base, vs->time_base);
        pack.dts = av_rescale_q(pack.dts, codec_context->time_base, vs->time_base);
        pack.duration = av_rescale_q(pack.duration, 
                                        codec_context->time_base, 
                                        vs->time_base);
        ret_code = av_interleaved_write_frame(format_context, &pack);
        if (0 == ret_code) { cout << "#" << flush; }
    }

    destroyAllWindows();
}

// 双边磨皮算法
int bilateral() {
	Mat src = imread("001.jpg");
	
	if (! src.data) {
		cout << "open file failed" << endl;
		getchar();

		return -1;
	}

	const int width		= 400;
	const int height	= 374;

	cv::namedWindow("src");
	cv::moveWindow("src", width, height);
	cv:imshow("src", src);

	Mat image;
	int d = 3;

	cv::namedWindow("image");
	cv::moveWindow("image", width, height);

	for (;;) {

		long long b = getTickCount();
		bilateralFilter(src, image, d, d*2, d/2);
		double sec = (double)(getTickCount()-b)/(double)getTickFrequency();
		cout << "d=" << d << " sec is " << sec << endl;

		imshow("image", image);

		int k = waitKey(0);
		if (k == 'd') {
			d += 2;
		} else if (k == 'f') {
			d -= 2;
		} else {
			break;
		}
	}

	waitKey(0);

	return 0;
}

int main(int argc, char *argv[])
{
    // showimage();

    // padding_rgb();

    // open_usb_camera();

    // opencv_rtmp();

	bilateral();

    getchar();

    return 0;
}
