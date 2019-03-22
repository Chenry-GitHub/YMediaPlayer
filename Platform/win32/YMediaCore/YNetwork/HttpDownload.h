#pragma once

#include "BaseHttpASync.h"

#include <string>
#include <functional>
class HttpDownload :public BaseHttpAsync
{

public:
	virtual bool OnAnalysis(const std::string &data) override
	{
		return true;
	}


	virtual void OnAfterHandleReply(bool) override
	{

	}


	int total_=0;
	std::function<void (float)> func_=nullptr;
	virtual void OnDataProgress(double total, double now) override
	{
		printf("OnDataProgress %f,%f\n",total,now);
		total_ = total;
		if (func_)
		{
			func_((float)now/total);
		}
	}


	int GetFileLength()
	{
		return total_;
	}
};