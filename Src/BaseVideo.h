#pragma once

#include <atomic>
#include <thread>
#include "BaseControl.h"

class BaseVideo:public BaseControl
{
public:
	BaseVideo() {}
	~BaseVideo(){}

	virtual void SetDisplay(void *) = 0;

	void Seek(float percent)
	{
		clock_ = duration_*percent;
	}

	double GetClock()
	{
		return clock_;
	}

	void SetDuration(double dur)
	{
		duration_= dur;
	}

	inline void SetBlockSeekFunction(std::function<bool()> func) {
		seek_func_ = func;
	}

	inline void SetDataFunction(std::function <bool(char ** data, int *width, int *height,double *pts)> func)
	{
		data_func_ = func;
	}

	inline void SetSyncToAudioFunction(std::function <bool()> func)
	{
		sync_func_ = func;
	}


protected:


	

	std::function<bool()> seek_func_;

	std::function <bool(char ** data, int *width, int *height, double *pts)> data_func_;

	std::function <bool()> sync_func_;
};

