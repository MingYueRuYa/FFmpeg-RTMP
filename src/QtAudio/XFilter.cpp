#include "XFilter.h"
#include "XBilateralFilter.h"

#include <iostream>

using std::cout;
using std::endl;

bool XFilter::Set(std::string key, double value) {

	if (paras.find(key) == paras.end()) {
		cout << "paras " << key << "is not support!" << endl;

		return false;
	}

	paras[key] = value;
	return true;
}

XFilter * XFilter::Get(XFilterType t) {
	static XBilateralFilter xbf;
	switch (t)
	{
	case XBILATERAL: //Ë«±ßÂË²¨
		return &xbf;
		break;
	default:
		break;
	}
	return 0;

}
XFilter::XFilter() {
}


XFilter::~XFilter() {
}

