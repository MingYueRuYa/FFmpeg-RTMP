#include "XRtmp.h"
#include "common.h"

#include <iostream>
#include <string>

using std::cout;
using std::endl;
using std::flush;
using std::string;

extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/time.h>
}

#pragma comment(lib, "avformat.lib")

class CXRtmp : public XRtmp
{
public:
    void Close() {
        if (ic) { 
            avformat_close_input(&ic); 
            vs = NULL;
        }

        vc = NULL;
        url = "";
    }

    bool Init(const char *url) {
        this->url = url;

        int ret = avformat_alloc_output_context2(&ic, 0, "flv", url);

        if (0 != ret) {
            av_error(ret);
            return false;
        }
        
        return true;
    }

    int AddStream(const AVCodecContext *c)
    {
        if (nullptr == c) { return -1; }

        AVStream *st = avformat_new_stream(ic, NULL);
        if (nullptr == st) { 
            cout << "add stream error" << endl;
            return -1;
        }

        st->codecpar->codec_tag = 0;

        // 编码器复制参数
        avcodec_parameters_from_context(st->codecpar, c);

        // 打印出详细信息
        av_dump_format(this->ic, 0, this->url.c_str(), 1);

        if (AVMEDIA_TYPE_VIDEO == c->codec_type) {
            this->vc = c;
            this->vs = st;
        } else if (AVMEDIA_TYPE_AUDIO == c->codec_type) {
            this->ac = c;
            this->as = st;
        }

        return st->index;
    }

    bool SendHead() {
        // 打开RTMP 的网络输出IO
        int ret = avio_open(&ic->pb, this->url.c_str(), AVIO_FLAG_WRITE);
        if (0 != ret) {
            av_error(ret);
            return false;
        }

        // 写入封装头
        ret = avformat_write_header(this->ic, NULL);
        if (0 != ret) {
            av_error(ret);
            return false;
        }

        return true;
    }

    bool SendFrame(AVPacket *packet, int streamIndex) {
        if (nullptr == packet) { return false; } 

        if (packet->size <= 0 || nullptr == packet->data) { return false; }

        packet->stream_index = streamIndex;

        AVRational stime;
        AVRational dtime;

        // 判断是音频还是视频
        if (vs && vc && packet->stream_index == vs->index) {
            stime = vc->time_base;
            dtime = vs->time_base;
        } else if (as && ac && packet->stream_index == as->index) {
            stime = ac->time_base;
            dtime = as->time_base;
        } else {
            return false;
        }

        packet->pts = av_rescale_q(packet->pts, stime, dtime);
        packet->dts = av_rescale_q(packet->dts, stime, dtime);
        packet->duration = av_rescale_q(packet->duration, stime, dtime);

        int ret = av_interleaved_write_frame(this->ic, packet);

        if (0 != ret) {
            av_error(ret);
            return false;
        } else {
            cout << "#" << flush;
        }

        return true;
    }

public:
    // rtmp flv 封装器
    AVFormatContext *ic = NULL;

    // 视频编码器流
    const AVCodecContext *vc = NULL;
    // 视频流
    AVStream *vs = NULL;

    string url = "";

    // 音频编码器
    const AVCodecContext *ac = NULL;
    // 音频流
    AVStream *as = NULL;
};


XRtmp::XRtmp() {
}

XRtmp* XRtmp::Get(unsigned char index) {
    static CXRtmp cxr[255];

    static bool is_first = true;

    if (is_first) {
        av_register_all();

        avformat_network_init();

        is_first = false;
    }

    return &cxr[index];
}

XRtmp::~XRtmp() {
}
