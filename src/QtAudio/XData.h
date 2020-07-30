#pragma once

class XData
{
public:
	char *data = 0;
	int size = 0;
	long long pts = 0;

	void Drop();

	XData();
	//创建空间，并复制data内容
	XData(char *data, int size,long long p=0);

	virtual ~XData();
};


//获取当前时间戳（微妙）
long long GetCurTime();