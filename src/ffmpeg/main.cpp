// ffmpeg.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include <string>
#include <iostream>
#include <functional>

#include <Windows.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

#pragma comment(lib, "avutil")
#pragma comment(lib, "avdevice")
#pragma comment(lib, "avcodec")
#pragma comment(lib, "avformat")
#pragma comment(lib, "avdevice")
#pragma comment(lib, "swscale")
#pragma comment(lib, "swresample")

using std::cout;
using std::endl;
using std::string;

static string dup_wchar_to_utf8(wchar_t *src)
{
	int length	= WideCharToMultiByte(CP_UTF8, 0, src, -1, 0, 0, 0, 0);
	char *dst	= (char *) av_malloc(length);
	if (nullptr != dst) {
		WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, length, 0, 0);
	}
	string encode_name = dst;
	av_free(dst);
	return encode_name;
}

//Show Dshow Device
void show_dshow_device(){
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    AVDictionary* options = nullptr;
    av_dict_set(&options,"list_devices","true",0);
    AVInputFormat *iformat = av_find_input_format("dshow");
    printf("========Device Info=============\n");
    avformat_open_input(&pFormatCtx,"video=dummy",iformat,&options);
    printf("================================\n");
}

//Show Dshow Device Option
void show_dshow_device_option(){
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    AVDictionary* options = nullptr;
    av_dict_set(&options,"list_options","true",0);
    AVInputFormat *iformat = av_find_input_format("dshow");
    printf("========Device Option Info======\n");
    avformat_open_input(&pFormatCtx,"video=Integrated Camera",iformat,&options);
    printf("================================\n");
}

//Show AVFoundation Device
void show_avfoundation_device(){
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    AVDictionary* options = nullptr;
    av_dict_set(&options,"list_devices","true",0);
    AVInputFormat *iformat = av_find_input_format("avfoundation");
    printf("==AVFoundation Device Info===\n");
    avformat_open_input(&pFormatCtx,"",iformat,&options);
    printf("=============================\n");
}

struct ResourceContainer {
	FILE *file_handle				= nullptr;
	AVCodecContext *codec_context	= nullptr;
	AVFrame *frame					= nullptr;
	AVFormatContext *format_context = nullptr;
};

int av_error(int errorCode) {
    char buff[1024] = {0};
    av_strerror(errorCode, buff, 1024);
    cout << buff << endl;
    return -1;
}

int _tmain(int argc, char* argv[])
{
    av_register_all();
    avdevice_register_all();

	// 这里获取你的音频设备名称
    show_dshow_device();

	// 资源容器类
	ResourceContainer containter;

	// 释放资源函数
	std::function<void(void)> release_resource = [&containter] () {
			if (nullptr != containter.file_handle) { 
				fclose(containter.file_handle); 
			}

			if (nullptr != containter.frame) {
				av_free(containter.frame);
				containter.frame = nullptr;
			}

			if (nullptr != containter.codec_context) {
				avcodec_close(containter.codec_context);
			}

			avformat_close_input(&(containter.format_context));
			avformat_free_context(containter.format_context);
	};

	// 分配上下文
    AVFormatContext	*format_context = avformat_alloc_context();
	containter.format_context = format_context;

	// 需要录制的设备名称，每台机器的可能不一样，需要替换成你自己的设备名称
	string device_name = dup_wchar_to_utf8(
						L"audio=立体声混音 (2- Realtek High Definition Audio)");
    
	int ret_code = 0;
	// 获取输入格式信息
    AVInputFormat *input_format = av_find_input_format("dshow");
	// 打开设备
    if(0 != (ret_code = avformat_open_input(&format_context, 
								 device_name.c_str(), 
								 input_format, 
								 nullptr))) {
		cout << av_error(ret_code) << endl;
		goto Error;
    }

    int audioindex = -1;

	// 遍历寻找是否有音频流信息
	for(int i = 0; i < format_context->nb_streams; i++) {
        if(format_context->streams[i]->codec->codec_type ==
														AVMEDIA_TYPE_AUDIO) {
            audioindex = i;
            break;
        }
	}

    if(-1 == audioindex) {
		cout << "Didn't find a audio stream." << endl;
		goto Error;
    }

	// 编码上下文
    AVCodecContext *codec_context = format_context->streams[audioindex]->codec;
	containter.codec_context	  = codec_context;
	// 寻找解码器
    AVCodec *codec = avcodec_find_decoder(codec_context->codec_id);

    if(nullptr == codec) {
		cout << "audio codec not found." << endl;
		goto Error;
    }

    if(0 != (ret_code = avcodec_open2(codec_context, codec, nullptr))) {
		cout << av_error(ret_code) << endl;
		goto Error;
    }

    AVFrame *frame		= av_frame_alloc();
	containter.frame	= frame;

    AVPacket *packet	= (AVPacket *)av_malloc(sizeof(AVPacket));
    FILE *fp_pcm		= fopen("output.pcm", "wb");
	containter.file_handle = fp_pcm;

    float total_time	= 0;
    int got_frame		= 0;

    for(int i = 0; ; i++) {
		//就采集30秒
		if (total_time > 30)  { break; }

		// 读取音频信息失败，直接退出
        if(av_read_frame(format_context, packet) < 0) { break; }

        if(packet->stream_index == audioindex) {
			// 对音频进行解码
            if (0 != (ret_code = avcodec_decode_audio4(codec_context, 
										frame, 
										&got_frame, 
										packet))) {

				cout << av_error(ret_code) << endl;
				goto Error;
			}

			if (0 == got_frame) { continue; }

			int pcm_size = 
					av_samples_get_buffer_size(nullptr, 
											   codec_context->channels, 
											   frame->nb_samples, 
											   codec_context->sample_fmt, 
											   1);
			uint8_t *pcm_buffer = frame->data[0];

			float used_time = 
					frame->nb_samples * 1.0 / codec_context->sample_rate;

			total_time += used_time;

			// 保存到文件中
			fwrite(pcm_buffer, 1, pcm_size, fp_pcm); 
        }
        av_free_packet(packet);
    }

	release_resource();
    return 0;

Error:
	release_resource();
	return -1;
}
