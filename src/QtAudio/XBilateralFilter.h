#ifndef xbilateralfilter_h
#define xbilateralfilter_h

#include "XFilter.h"

class XBilateralFilter : public XFilter
{
public:
	XBilateralFilter();
	bool Filter(cv::Mat *src, cv::Mat *des);
	virtual ~XBilateralFilter();
};

#endif // xbilateralfilter_h