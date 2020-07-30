#include "XData.h"

#include <stdlib.h>
#include <string.h>

extern "C"
{
#include <libavutil/time.h>
}

long long GetCurTime()
{
	return av_gettime();
}

void XData::Drop() {
    if (data) { delete data; }
	data = 0;
	size = 0;
}

XData::XData(char *data, int size, long long p) {
	this->data = new char[size];
	memcpy(this->data, data, size);
	this->size = size;
	this->pts = p;
}

XData::XData(){
}

XData::~XData(){
}
