#pragma once

#include <atomic>
#include <thread>
#include "BaseControl.h"

class BaseVideo :public BaseControl
{
public:
	class Delegate
	{
	public:
		virtual bool onVideoSeek() = 0;
		virtual bool onVideoGetData(char ** data, int *width, int *height, double *pts) = 0;
		virtual bool onVideoSync() = 0;
		virtual void onVideoDisplay(void *data, int width, int height) = 0;
	};

	virtual void setDelegate(BaseVideo::Delegate * dele) { delegate_ = dele; };
	virtual BaseVideo::Delegate * getDelegate() {return delegate_;};

protected:
	BaseVideo::Delegate * delegate_ = nullptr;
};

