#ifndef ffmpeg_common_h
#define ffmpeg_common_h

extern "C"
{
#include "libavformat/avformat.h"
}

 #define has_error(ret_code) {                              \
                                if (0 != ret_code) {        \
                                    av_error(ret_code);     \
                                    return false;           \
                                }                           \
                             }

int av_error(int errorCode);

double r2d(AVRational r);

#endif // ffmpeg_common_h