#include "common.h"

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QtMultimedia/QAudioInput>

#include <iostream>

extern "C"
{
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#pragma comment(lib, "swresample.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avformat.lib")

using std::cout;
using std::endl;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

	//注册所有的编解码器
	avcodec_register_all();

	//注册所有的封装器
	av_register_all();

	//注册所有网络协议
	avformat_network_init();

    char *out_url = "rtmp://192.168.26.31/live";

    int sample_rate = 44100;
    int channels    = 2;
    int sample_byte = 2;

    AVSampleFormat in_sample_fmt    = AV_SAMPLE_FMT_S16;
    AVSampleFormat out_sample_fmt   = AV_SAMPLE_FMT_FLTP;

    QAudioFormat audio_format;
    audio_format.setSampleRate(sample_rate);
    audio_format.setChannelCount(channels);
    audio_format.setSampleSize(sample_byte*8);
    audio_format.setCodec("audio/pcm");
    audio_format.setByteOrder(QAudioFormat::LittleEndian);
    audio_format.setSampleType(QAudioFormat::UnSignedInt);

    QAudioDeviceInfo device_info = QAudioDeviceInfo::defaultInputDevice();

    if (! device_info.isFormatSupported(audio_format)) {
        cout << "Audio format not support " << endl;
        return -1;
    }

    QAudioInput *input = new QAudioInput(audio_format);
    // 开始录制音频
    QIODevice *io = input->start();

    // 2 音频重采样，上下文初始化
    SwrContext *asc = nullptr;
    asc = swr_alloc_set_opts(asc,
        // 输出格式
        av_get_default_channel_layout(channels), out_sample_fmt, sample_rate, 
        // 输入格式
        av_get_default_channel_layout(channels), in_sample_fmt, sample_rate, 
        0, 0);
    if (nullptr == asc) {
        cout << "swr_alloc_set_opts failed.\n";
        return -1;
    }

    int ret = swr_init(asc);
    has_error(ret);

    cout << "音频重采样，上下文初始化成功！" << endl;

    // 3 音频重采样输出空间分配
    AVFrame *pcm    = av_frame_alloc();
    pcm->format     = out_sample_fmt;
    pcm->channels   = channels;
    pcm->channel_layout = av_get_default_channel_layout(channels);
    // 一帧音频一通道的采用数量
    pcm->nb_samples = 1024;
    // pcm分配存储空间
    ret = av_frame_get_buffer(pcm, 0);
    has_error(ret);

    // 4 初始化音频编码器
    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (nullptr == codec) {
        cout << "avcodec_find_encoder AV_CODEC_ID_AAC failed." << endl;
        return -1;
    }

    // 音频编码器上下文
    AVCodecContext *ac = avcodec_alloc_context3(codec);
    if (nullptr == ac) {
        cout << "avcodec_alloc_context3 failed." << endl;
        return -1;
    }

    cout << "avcodec_alloc_context3 successful." << endl;

    ac->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    ac->thread_count = 8;
    ac->bit_rate = 40000;
    ac->sample_rate = sample_rate;
    ac->sample_fmt = AV_SAMPLE_FMT_FLTP;
    ac->channels = channels;
    ac->channel_layout = av_get_default_channel_layout(channels);

    // 打开音频编码器
    ret = avcodec_open2(ac, 0, 0);
    has_error(ret);

    cout << "avcodec_open2 successful." << endl;

    // 5 输出封装器和音频流配置
    // a. 创建输出封装器上下文
    AVFormatContext *ic = nullptr;
    ret = avformat_alloc_output_context2(&ic, 0, "flv", out_url);
    has_error(ret);

    // b.添加音频流
    AVStream *as = avformat_new_stream(ic, nullptr);
    if (nullptr == as) {
        cout << "avformat_new_stream failed" << endl;
        return -1;
    }

    as->codecpar->codec_tag = 0;

    // 从编码器复制参数
    avcodec_parameters_from_context(as->codecpar, ac);
    av_dump_format(ic, 0, out_url, 1);

    // 打开rtmp的网络输出io
    ret = avio_open(&ic->pb, out_url, AVIO_FLAG_WRITE);
    has_error(ret);

    // 写入封装头
    ret = avformat_write_header(ic, nullptr);
    has_error(ret);

    // 一次读取一帧音频的字节数
    int read_size = pcm->nb_samples * channels * sample_byte;
    char *buf = new char[read_size];
    int apts = 0;
    AVPacket pkt = {0};
    for (;;) {
        // 一次读取一帧音频
        if (input->bytesReady() < read_size) {
            QThread::msleep(1); 
            continue;
        }

        int size = 0;
        while (size != read_size) {
            int len = io->read(buf+size, read_size-size);
            if (len < 0) { break; }
            size += len;
        }

        if (size != read_size) { continue; }

        // 已经读取一帧数据
        // 重采样源数据
        const uint8_t *indata[AV_NUM_DATA_POINTERS] = {0};
        indata[0] = (uint8_t *)buf;
        int len = swr_convert(
                                // 输出参数，输出存储地址和样本数量
                                asc, pcm->data, pcm->nb_samples,
                                indata, pcm->nb_samples);

        // pts运算
        // nb_sample/sample_rate = 一帧音频的秒数sec
        // timebase pts = sec*timebaes.den
        pcm->pts = apts;
        apts += av_rescale_q(pcm->nb_samples, {1, sample_rate}, ac->time_base);

        int ret = avcodec_send_frame(ac, pcm);
        if (0 != ret) { continue; }

        av_packet_unref(&pkt);

        ret = avcodec_receive_packet(ac, &pkt);
        if (0 != ret) { continue; }

        cout << pkt.size << " " << flush;

        // 推流
        pkt.pts = av_rescale_q(pkt.pts, ac->time_base, as->time_base);
        pkt.dts = av_rescale_q(pkt.dts, ac->time_base, as->time_base);
        pkt.duration = av_rescale_q(pkt.duration, ac->time_base, as->time_base);
        ret = av_interleaved_write_frame(ic, &pkt);
        if (0 == ret) { cout << "#" << flush; }
            
    }

    delete[] buf;

    /*
    QList<QAudioDeviceInfo> device_list = 
                        QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    for each (QAudioDeviceInfo info in device_list)
    {
        qDebug() << info.deviceName() << "\n";
    }
    */

    return a.exec();
}
