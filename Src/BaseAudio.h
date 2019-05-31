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
	class Delegate
	{
	public:
		virtual bool onAudioGetData(char ** data, int *len, double *pts) = 0;
		virtual void onAudioCurrent(int) = 0;
		virtual void onAudioSeek() = 0;
		virtual void onAudioFreeData(char*) = 0;
	};

	virtual void setDelegate(BaseAudio::Delegate* dele) { delegate_ = dele; };
	virtual BaseAudio::Delegate* getDelegate() {return delegate_;};

protected:
	BaseAudio::Delegate* delegate_=nullptr;
};

