#pragma once

#include <thread>
#include <atomic>

class BaseControl
{
public:
	BaseControl()
		:is_playing_(false)
		,is_stop_(true)
		, clock_(0)
		,duration_(0)
	{

	}
	virtual ~BaseControl() {}

	void BeginPlayThread()
	{
		is_stop_ = false;
		play_thread_ = std::move(std::thread(&BaseControl::PlayThread, this));
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

	virtual void Play()
	{
		is_stop_ = false;
		is_playing_ = true;
	}

	virtual void Pause()
	{
		is_playing_ = false;
	}

	virtual void Seek(float percent)
	{
		clock_ = duration_*percent;
	}

	virtual void Stop()
	{
		clock_ = 0.0f;
		duration_ = 0.0f;
		is_playing_ = false;
		is_stop_ = true;
	}

	void SetDuration(double dur)
	{
		duration_ = dur;
	}


	bool IsPlaying()
	{
		return is_playing_;
	}

	bool IsStop()
	{
		return is_stop_;
	}

	double GetClock()
	{
		return clock_;
	}

protected:
	std::atomic_bool is_playing_;
	std::atomic_bool is_stop_;
	double clock_;
	double duration_;

	virtual void PlayThread() = 0;

	std::thread play_thread_;
};
