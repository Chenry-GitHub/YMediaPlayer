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

	void beginPlayThread()
	{
		is_stop_ = false;
		play_thread_ = std::move(std::thread(&BaseControl::playThread, this));
	}

	void endPlayThread()
	{
		is_playing_ = false;
		if (play_thread_.joinable())
		{
			is_stop_ = true;
			play_thread_.join();
		}
	}

	virtual void play()
	{
		is_stop_ = false;
		is_playing_ = true;
	}

	virtual void pause()
	{
		is_playing_ = false;
	}

	virtual void seek(float percent)
	{
		clock_ = duration_*percent;
	}

	virtual void stop()
	{
		clock_ = 0.0f;
		duration_ = 0.0f;
		is_playing_ = false;
		is_stop_ = true;
	}

	void setDuration(double dur)
	{
		duration_ = dur;
	}


	bool isPlaying()
	{
		return is_playing_;
	}

	bool isStop()
	{
		return is_stop_;
	}

	double getClock()
	{
		return clock_;
	}

protected:
	virtual void playThread() = 0;

	std::atomic_bool is_playing_;
	std::atomic_bool is_stop_;
	double clock_ = 0.0f;
	double duration_ = 0.0f;
	std::thread play_thread_;
};
