#pragma once

#include <atomic>
#include <thread>

class BaseVideo
{
public:
	BaseVideo() {}
	~BaseVideo(){}

	virtual void SetDisplay(void *) = 0;

	void BeginPlayThread()
	{
		is_stop_ = false;
		play_thread_ = std::move(std::thread(&BaseVideo::PlayThread, this));
	}

	void EndPlayThread()
	{
		if (play_thread_.joinable())
		{
			is_stop_ = true;
			play_thread_.join();
		}
	}

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

	void Stop()
	{
		is_stop_ = true;
	}

	inline void SetBlockSeekFunction(std::function<bool()> func) {
		seek_func_ = func;
	}

	inline void SetDataFunction(std::function <bool(char ** data, int *width, int *height,double *pts)> func)
	{
		data_func_ = func;
	}

	inline void SetSyncToAudioFunction(std::function <void()> func)
	{
		sync_func_ = func;
	}


protected:

	virtual void PlayThread() =0;

	double clock_;
	double duration_;
	std::atomic_bool is_stop_;

	std::thread play_thread_;

	std::function<bool()> seek_func_;

	std::function <bool(char ** data, int *width, int *height, double *pts)> data_func_;

	std::function <void()> sync_func_;
};

