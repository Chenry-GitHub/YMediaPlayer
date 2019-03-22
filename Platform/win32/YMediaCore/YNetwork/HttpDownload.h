#pragma once

#include "BaseHttpASync.h"

#include <string>

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
	virtual void OnDataProgress(double total, double now) override
	{
		printf("OnDataProgress %f,%f\n",total,now);
		total_ = total;
	}


	int GetFileLength()
	{
		return total_;
	}
};