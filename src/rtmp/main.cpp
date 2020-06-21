#include "common.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/time.h"
}

#include <iostream>

using std::cout;
using std::endl;

#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")

int main(int argc, char *argv[])
{
    char *in_url = "test.flv";
    // 初始化所有封装和解封装 flv mp4 mov mp3
    av_register_all();

    // 初始化网络库
    avformat_network_init();

    ///////////////////////////////////////////////////////////////////
    // 输入流打开文件，解封装
    // 输入封装上下文
    AVFormatContext *ictx = NULL;

    // 打开文件，解封文件头
    int ret_code = avformat_open_input(&ictx, in_url, 0, 0);
    if (ret_code < 0) { return av_error(ret_code); }

    // 获取音视频流信息，h264 flv
    ret_code = avformat_find_stream_info(ictx, 0);
    if (ret_code < 0) { return av_error(ret_code); }

    cout << "---------------------------------" << endl;
    av_dump_format(ictx, 0, in_url, 0);
    cout << "---------------------------------" << endl;

    ///////////////////////////////////////////////////////////////////
    // 输出流
    char *out_url = "rtmp://192.168.26.31/live";
    AVFormatContext *octx = NULL;
    ret_code = avformat_alloc_output_context2(&octx, 0, "flv", out_url);
    if (NULL == octx) { return av_error(ret_code); }

    for (int i = 0; i < ictx->nb_streams; ++i) {
        // 创建输出流
        AVStream *out = avformat_new_stream(octx, ictx->streams[i]->codec->codec);
        if (NULL == out) { return av_error(0); }

        ret_code = avcodec_parameters_copy(out->codecpar, ictx->streams[i]->codecpar);
        out->codec->codec_tag = 0;
    }

    cout << "---------------------------------" << endl;
    av_dump_format(octx, 0, out_url, 1);
    cout << "---------------------------------" << endl;


    ////////////////////////////////////////////////////////////////////////
    //RTMP推流
    ret_code = avio_open(&octx->pb, out_url, AVIO_FLAG_WRITE);
    if (NULL == octx->pb) { return av_error(ret_code); }

    // 写入头信息
    ret_code = avformat_write_header(octx, 0);
    if (ret_code < 0) { return av_error(ret_code); }
    cout << "avformat_write_header" << endl;

    AVPacket pkt;
    int64_t last_pts = 0;
    long long startTime = av_gettime();
    for (;;) {
        ret_code = av_read_frame(ictx, &pkt);

        if (0 != ret_code) { break; }
        
        // 跳过异常的pts
        if (last_pts > pkt.pts) { continue; }

        cout << pkt.pts << " " << std::flush;

        // 计算转换pts dts
        AVRational itime = ictx->streams[pkt.stream_index]->time_base;
        AVRational otime = octx->streams[pkt.stream_index]->time_base;
        pkt.pts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_NEAR_INF));
        pkt.dts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_NEAR_INF));
        pkt.duration = av_rescale_q_rnd(pkt.duration, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_NEAR_INF));
        pkt.pos = -1;
        last_pts = pkt.pts;

        // 视频帧推送速度
        if (ictx->streams[pkt.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVRational tb = ictx->streams[pkt.stream_index]->time_base;
            // 过去的时间
            long long now = av_gettime()-startTime;
            long long dts = 0;
            dts = pkt.dts * (1000 * 1000 * r2d(tb));

            if (dts > now) { av_usleep(dts-now); }
        }

        ret_code = av_interleaved_write_frame(octx, &pkt);
        // 注意出错不用直接返回，否则推流会直接结束
        if (ret_code < 0) { /*return av_error(ret_code);*/ }
    }

    getchar();

    return 0;
}