#ifndef ffmpeg_common_h
#define ffmpeg_common_h

extern "C"
{
#include "libavformat/avformat.h"
}

int av_error(int errorCode);

double r2d(AVRational r);

#endif // ffmpeg_common_h