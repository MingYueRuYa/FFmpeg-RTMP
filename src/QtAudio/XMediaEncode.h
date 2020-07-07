#pragma once

struct AVFrame;
struct AVPacket;

class AVCodecContext;

enum XSampleFMT {
    X_S16 = 1,
    X_FLATP = 8
};

// 音视频编码接口类
class XMediaEncode
{
public:
    // 输入参数
    int in_width    = 1280;
    int in_height   = 720;
    int pixsize     = 3;
    int channels    = 2;
    int sample_rate = 44100;
    XSampleFMT in_smaple_fmt = X_S16;

    // 输出参数
    int out_width   = 1280;
    int out_height  = 720;
    // 压缩后每秒视频的bit位大小50K
    int bitrate     = 4000000; 
    int fps         = 25;
    int nb_sample   = 1024;
    XSampleFMT  out_sample_fmt = X_FLATP;

    static XMediaEncode *Get(unsigned char index = 0);

    // 初始化像素格式转换的上下文初始化
    virtual bool InitScale() = 0;

    // 音频重采样上下文初始化
    virtual bool InitResample() = 0;

    // 返回值 无需调用者清理
    virtual AVFrame *Resample(char *data) = 0;

    // 返回值 无需调用者清理
    virtual AVFrame *RGBToYUV(char *rgb) = 0;

    // 视频编码初始化
    virtual bool InitVideoCodec() = 0;

    // 音频编码初始化
    virtual bool InitAudioCode() = 0;

    // 视频编码
    virtual AVPacket *EncodeVideo(AVFrame *frame) = 0;

    // 音频编码
    virtual AVPacket *EncodeAudio(AVFrame *frame) = 0;

    virtual ~XMediaEncode();

    // 视频编码器上下文
    AVCodecContext *vc = 0;

    // 音频编码器上下文
    AVCodecContext *ac = 0;

protected:
    XMediaEncode();
};

