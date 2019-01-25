#pragma once

#include <string>
#include <functional>
#include <atomic>

#include "YMediaComm.h"
#include "BaseControl.h"

#define  AUDIO_OUT_SAMPLE_RATE 44100
#define  AUDIO_OUT_CHANNEL 2

class BaseAudio:public BaseControl
{
public:
	BaseAudio()
		:data_func_(nullptr)
		, status_func_(nullptr)
		, dur_func_(nullptr)
		, cur_func_(nullptr)
		, seek_func_(nullptr)
	{ 
		Stop(); 
	}
	~BaseAudio(){}


	inline void SetDataFunction(std::function <bool(char ** data, int *len, double *pts)> func)
	{
		data_func_ = func;
	}

	inline void SetBlockSeekFunction(std::function<bool()> func) {
		seek_func_ = func;
	}

	inline void SetStatusFunction(std::function<void(PlayerStatus)> func)
	{
		status_func_ = func;
	}

	inline void SetDurationFunction(std::function<void(int)> func)
	{
		dur_func_ = func;
	}

	inline void SetProgressFunction(std::function<void(int)> func)
	{
		cur_func_ = func;
	}

	inline void SetFreeDataFunction(std::function<void(char*)> func)
	{
		free_func_ = func;
	}

protected:


	std::function<bool (char ** data, int *len, double *pts)> data_func_;

	std::function<void(PlayerStatus)> status_func_;

	std::function<void(int)> dur_func_;

	std::function<void(int)> cur_func_;

	std::function<bool ()> seek_func_;

	std::function<void(char*)> free_func_;
};

