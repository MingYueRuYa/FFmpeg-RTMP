#ifndef xfilter_h
#define xfilter_h

#include <map>
#include <string>

enum XFilterType
{
	XBILATERAL // Ë«±ßÄ¥Æ¤
};

namespace cv {
class Mat;
}

class XFilter {
public:
	static XFilter *Get(XFilterType type = XBILATERAL);
	virtual bool Filter(cv::Mat *src, cv::Mat *des) = 0;
	virtual bool Set(std::string key, double value);
	virtual ~XFilter();

protected:
	std::map<std::string ,double> paras;
	XFilter();

};

#endif // xfilter_h