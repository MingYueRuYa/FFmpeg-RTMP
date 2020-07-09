#pragma once

#include "XDataThread.h"

enum XAUDIOTYPE
{
	X_AUDIO_QT
};

class XAudioRecord:public XDataThread
{
public:
	int channels = 2;		// 通道数
	int sampleRate = 44100;	// 采样率
	int sampleByte = 2;		// 样本字节大小
	int nbSamples = 1024;	// 一帧音频每个通道的样本数量
	// 开始录制
	virtual bool Init() = 0;

	// 停止录制
	virtual void Stop() = 0;

	static XAudioRecord *Get(XAUDIOTYPE type = X_AUDIO_QT, unsigned char index = 0);
	virtual ~XAudioRecord();
protected:
	XAudioRecord();
};
