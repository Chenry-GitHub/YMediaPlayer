#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>

#include "YMediaComm.h"

class BaseAudio
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

	void BeginPlayThread()
	{
		is_stop_ = false;
		play_thread_ = std::move(std::thread(&BaseAudio::PlayThread, this));
	}

	void EndPlayThread()
	{
		is_playing_ = false;
		if (play_thread_.joinable())
		{
			is_stop_ = true;
			play_thread_.join();
		}
	}

	virtual void Stop()
	{
		clock_ = 0.0f;
		duration_ = 0.0f;
		is_playing_ = false;
		is_stop_ = true;
	}

	virtual void Play()
	{
		is_playing_ = true;
	}

	virtual void Pause()
	{
		is_playing_ = false;
	}

	bool IsStop()
	{
		return is_stop_;
	}

	virtual void Seek(float percent)
	{
		clock_= duration_*percent;
	}

	bool IsPlaying()
	{
		return is_playing_;
	}

	double GetClock()
	{
		return clock_;
	}

	void SetDuration(double dur)
	{
		duration_ = dur;
	}

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

protected:

	virtual void PlayThread() = 0;

	double clock_;
	
	double duration_;

	std::atomic_bool  is_playing_;

	std::atomic_bool is_stop_;

	std::function<bool (char ** data, int *len, double *pts)> data_func_;

	std::function<void(PlayerStatus)> status_func_;

	std::function<void(int)> dur_func_;

	std::function<void(int)> cur_func_;

	std::function<bool ()> seek_func_;

	std::thread play_thread_;


};

