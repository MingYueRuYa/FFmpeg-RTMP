#ifndef xvideocapture_h
#define xvideocapture_h

#include "XDataThread.h"
#include "XFilter.h"
#include <vector>
class XVideoCapture : public XDataThread
{
public:
    int width   = 0;
    int height  = 0;
    int fps     = 0;

    static XVideoCapture *Get(unsigned char index = 0);
    virtual bool Init(int camIndex = 0) = 0;
    virtual bool Init(const char *url) = 0;
    virtual void Stop() = 0;

    virtual ~XVideoCapture();
	void AddFilter(XFilter *f)
	{
		fmutex.lock();
		filters.push_back(f);
		fmutex.unlock();
	}
protected:
	QMutex fmutex;
	std::vector<XFilter*> filters;
    XVideoCapture();

};

#endif // xvideocapture_h