#pragma once

struct AVFrame;
struct AVPacket;
struct AVCodecContext;

// 音视频编码接口类
class XMediaEncode
{
public:
    // 输入参数
    int in_width    = 1280;
    int in_height   = 720;
    int pixsize     = 3;

    // 输出参数
    int out_width   = 1280;
    int out_height  = 720;
    // 压缩后每秒视频的bit位大小50K
    int bitrate     = 4000000; 
    int fps         = 25;

    static XMediaEncode *Get(unsigned char index = 0);

    // 初始化像素格式转换的上下文初始化
    virtual bool InitScale() = 0;

    virtual AVFrame *RGBToYUV(char *rgb) = 0;

    // 视频编解码初始化
    virtual bool InitVideoCodec() = 0;

    // 视频编码
    virtual AVPacket *EncodeVideo(AVFrame *frame) = 0;

    virtual ~XMediaEncode();

    // 编码器上下文
    AVCodecContext *vc = 0;

public:
    XMediaEncode();
};

