#include "common.h"

#include <iostream>

using std::cout;
using std::endl;

int av_error(int errorCode) {
    char buff[1024] = {0};
    av_strerror(errorCode, buff, 1024);
    cout << buff << endl;
    return -1;
}

double r2d(AVRational r) {
    return r.num == 0 || r.den == 0 ?
            0. : static_cast<double>(r.num) / static_cast<double>(r.den);
}
