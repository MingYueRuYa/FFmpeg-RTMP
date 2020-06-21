#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
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

void opencv_rtmp() {
    char *out_url = "rtmp://192.168.26.31/live";

    // 注册所有的编解码器
    avcodec_register_all();

    // 注册所有的封装器
    av_register_all();

    // 注册所有网络协议
    avformat_network_init();

    VideoCapture camera;
    Mat frame;
    namedWindow("vedio");

    // 像素格式转换上下文
    SwsContext *vsc = NULL;

    // 输出的数据结构
    AVFrame *yuv = NULL;

    // 编码器上下文
    AVCodecContext *vc = NULL;

    // rtmp flv封装器
    AVFormatContext *ic = NULL;

    try {
        // 1.使用opencv 打开usb 摄像头
        camera.open(0);

        if (! camera.isOpened()) { 
            throw std::exception("camera open usb camera error"); 
        }

        cout << "open usb camera successful." << endl;

        int width   = camera.get(CAP_PROP_FRAME_WIDTH);
        int height  = camera.get(CAP_PROP_FRAME_HEIGHT);
        int fps     = camera.get(CAP_PROP_FPS);

        if (0 == fps) { fps = 25; }

        // 2.初始化格式转换上下文
        vsc = sws_getCachedContext(vsc,
                        width, height, AV_PIX_FMT_BGR24,    // 源格式
                        width, height, AV_PIX_FMT_YUV420P,  // 目标格式
                        SWS_BICUBIC,    // 尺寸变化使用算法
                        0, 0, 0);

        if (NULL == vsc) { throw std::exception("sws_getCachedContext error"); }

        // 3.初始化输出的数据结构
        yuv         = av_frame_alloc();
        yuv->format = AV_PIX_FMT_YUV420P;
        yuv->width  = width;
        yuv->height = height;
        yuv->pts    = 0;

        // 分配yuv空间
        int ret_code = av_frame_get_buffer(yuv, 32);
        if (0 != ret_code) {
            char buff[1024] = {0};
            av_strerror(ret_code, buff, sizeof(buff)-1);
            throw std::exception(buff);
        }

        // 4.初始化编码上下文
        // 4.1找到编码器
        AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (NULL == codec) { throw std::exception("Can't find h264 encoder."); }

        // 4.2创建编码器上下文
        vc = avcodec_alloc_context3(codec);
        if (NULL == vc) { 
           throw std::exception("avcodec_alloc_context3 failed."); 
        }

        // 4.3配置编码器参数
        // vc->flags           |= AV_CODEC_FLAG_GLOBAL_HEADER;
        vc->codec_id        = codec->id;
        vc->thread_count    = 8;

        // 压缩后每秒视频的bit流 50k
        vc->bit_rate = 50*1024*8;
        vc->width = width;
        vc->height = height;
        vc->time_base = {1, fps};
        vc->framerate = {fps, 1};

        // 画面组的大小，多少帧一个关键帧
        vc->gop_size = 50;
        vc->max_b_frames = 0;
        vc->pix_fmt = AV_PIX_FMT_YUV420P;
        vc->qmin = 10;
        vc->qmax = 51;

        // 4.4打开编码器上下文
        ret_code = avcodec_open2(vc, 0, 0);
        if (0 != ret_code) { 
            char buff[1024] = {0};
            av_strerror(ret_code, buff, sizeof(buff)-1);
            throw std::exception(buff);
        }
        cout << "avcodec_open2 success!" << endl;

        // 5.输出封装器和视频流配置
        // 5.1创建输出封装器上下文
        ret_code = avformat_alloc_output_context2(&ic, 0, "flv", out_url);
        if (0 != ret_code) { 
            char buff[1024] = {0};
            av_strerror(ret_code, buff, sizeof(buff)-1);
            throw std::exception(buff);
        }
        // 5.2添加视频流
        AVStream *vs = avformat_new_stream(ic, NULL);
        if (NULL == vs) { throw std::exception("avformat_new_stream failed."); }

        vs->codecpar->codec_tag = 0;
        // 从编码器复制参数
        avcodec_parameters_from_context(vs->codecpar, vc);
        av_dump_format(ic, 0, out_url, 1);

        // 打开rtmp 的网络输出IO
        ret_code = avio_open(&ic->pb, out_url, AVIO_FLAG_WRITE);
        if (0 != ret_code) { 
            char buff[1024] = {0};
            av_strerror(ret_code, buff, sizeof(buff)-1);
            throw std::exception(buff);
        }

        // 写入封装头
        ret_code = avformat_write_header(ic, NULL);
        if (0 != ret_code) { 
            char buff[1024] = {0};
            av_strerror(ret_code, buff, sizeof(buff)-1);
            throw std::exception(buff);
        }

        AVPacket pack;
        memset(&pack, 0, sizeof(pack));
        int vpts = 0;
        for (;;) {
            // 读取rtsp视频帧，解码视频帧
            if (! camera.grab()) { continue; }

            // yuv转换为rgb
            if (! camera.retrieve(frame)) { continue; }

            imshow("video", frame);
            waitKey(1);

            // rgb to yuv
            uint8_t *in_data[AV_NUM_DATA_POINTERS] = {0};
            in_data[0] = frame.data;
            int in_size[AV_NUM_DATA_POINTERS] = {0};
            // 一行（宽）数据的字节数
            in_size[0] = frame.cols * frame.elemSize();
            int h = sws_scale(vsc, in_data, in_size, 0, frame.rows,
                                yuv->data, yuv->linesize);
            if (h <= 0) { continue; }

            // h264编码
            yuv->pts = vpts;
            vpts++;

            ret_code = avcodec_send_frame(vc, yuv);
            if (0 != ret_code) { continue; }

            ret_code = avcodec_receive_packet(vc, &pack);
            
            if (0 != ret_code || pack.buf > 0) {
                //TODO something
            } else { continue; }

            // 推流
            pack.pts = av_rescale_q(pack.pts, vc->time_base, vs->time_base);
            pack.dts = av_rescale_q(pack.dts, vc->time_base, vs->time_base);
            pack.duration = av_rescale_q(pack.duration, 
                                            vc->time_base, 
                                            vs->time_base);
			ret_code = av_interleaved_write_frame(ic, &pack);
            if (0 == ret_code) { cout << "#" << flush; }
        }

    } catch (std::exception &ex) {
        if (camera.isOpened()) { camera.release(); }

		if (vsc) {
			sws_freeContext(vsc);
			vsc = NULL;
		}

        if (vc) {
            if (ic) { avio_closep(&ic->pb); }
			avcodec_free_context(&vc);
		}

		cerr << ex.what() << endl;
    }

    camera.release();
    destroyAllWindows();
}

int main(int argc, char *argv[])
{
    // showimage();

    // padding_rgb();

    // open_usb_camera();

    opencv_rtmp();

    getchar();

    return 0;
}
